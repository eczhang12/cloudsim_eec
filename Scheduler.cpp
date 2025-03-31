//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"
#include <atomic>         // For std::atomic<unsigned>
#include <thread>         // For std::this_thread::sleep_for()
#include <chrono>         // For std::chrono::milliseconds
#include <tuple>          // for your taskCombos map
#include <unordered_map>
#include <map>
#include <algorithm>



// struct MachineLoad {
//   MachineId_t id;
//   double utilization;
// };

// // Helper function to get the current utilization of a machine
// double GetMachineUtilization(MachineId_t machine) {
//   double used = Machine_GetUsedResources(machine);   // Current used resources
//   double capacity = Machine_GetTotalResources(machine);  // Machine capacity
//   return (capacity > 0) ? used / capacity : 0.0;
// }

// // Helper function to sort machines by utilization (ascending order)
// bool CompareByUtil(const MachineLoad &a, const MachineLoad &b) {
//   return a.utilization < b.utilization;
// }


typedef struct {
    vector<VMId_t> active_vms;
    vector<unsigned int> total_mips;
    bool changing_pstate; // is machine changing performance state
    bool changing_cstate; // is machine changing power state
  } MoreMachineInfo_t;

static bool migrating = false;
static std::unordered_map<MachineId_t, MoreMachineInfo_t> moreMachineInfo;
static unsigned total_machines;
static unsigned total_tasks;
static CPUPerformance_t def_cpu_pstate;
static std::map<unsigned int, std::vector<MachineId_t>> machines_by_mips;
static std::unordered_map<unsigned int, unsigned int> performance_indexRR;
/**
 * Stores the VM key and the memory that the VM takes up (8 + all the tasks required memory)
 */
static std::unordered_map<VMId_t, unsigned int> toMigrate;
/**
 * The idea behind pending migration is that when we call VM_migrate, it takes
 * some time for the VM to show up on the machine and thus the memory it will 
 * take up in the future does not show up. This is to keep track of what Vms
 * are currently waiting to be put on a specific machine
 */
static std::unordered_map<MachineId_t, std::vector<VMId_t>> pendingMigration;



void Scheduler::Init() {
  // Find the parameters of the clusters
  // Get the total number of machines
  // For each machine:
  //      Get the type of the machine
  //      Get the memory of the machine
  //      Get the number of CPUs
  //      Get if there is a GPU or not
  //
  
    SimOutput("Scheduler::Init(): Total number of machines is " +
                    to_string(Machine_GetTotal()),
                3);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);

    total_machines = Machine_GetTotal();
    def_cpu_pstate = P0;
    total_tasks = GetNumTasks();

    for (unsigned i = 0; i < total_machines; i++) {
        MachineInfo_t M_info = Machine_GetInfo(MachineId_t(i));
        cout << "just got info for machine: " << i << endl;
        MoreMachineInfo_t& moreM_info = moreMachineInfo[MachineId_t(i)];
        machines.push_back(MachineId_t(i));

        cout << "pushed back onto machines for machine: " << i << endl;

        // update total_mips
        unsigned int n_cpus = M_info.num_cpus;
        for (unsigned j = 0; j < 4; j++) {
            moreM_info.total_mips.push_back(unsigned(n_cpus * M_info.performance[j]));
        }

        //put machines into map based on power
        machines_by_mips[M_info.performance[0]].push_back(MachineId_t(i));
        performance_indexRR[M_info.performance[0]] = 0;

        // this machine isn't changing state
        moreM_info.changing_cstate = false;
        moreM_info.changing_pstate = false;

        // set core performance
        for (unsigned j = 0; j < Machine_GetInfo(MachineId_t(i)).num_cpus; j++) {
        Machine_SetCorePerformance(MachineId_t(i), j, CPUPerformance_t(def_cpu_pstate));
        }
    }
    cout << "get out of init" << endl;
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
  // Update your data structure. The VM now can receive new tasks
  //This vm has finished migrating
  //take the VM off the toMigrate list
  toMigrate.erase(vm_id);
  
  //update the Machine pending migration list to get rid of the VM that just got migrated
  pendingMigration[VM_GetInfo(vm_id).machine_id].erase(std::remove(pendingMigration[VM_GetInfo(vm_id).machine_id].begin(), pendingMigration[VM_GetInfo(vm_id).machine_id].end(), vm_id), pendingMigration[VM_GetInfo(vm_id).machine_id].end());

}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    // Decide to attach the task to an existing VM,
    //      vm.AddTask(taskid, Priority_T priority); or
    // Create a new VM, attach the VM to a machine
    //      VM vm(type of the VM)
    //      vm.Attach(machine_id);
    //      vm.AddTask(taskid, Priority_t priority) or
    // Turn on a machine, create a new VM, attach it to the VM, then add the task
    //
    // Turn on a machine, migrate an existing VM from a loaded machine....
    //
    // Other possibilities as desired
    

    /**
     * The general idea here is to add the tasks to the weakest machines first
     * We only migrate the VMs to the highest machines when they are in danger
     * of not meeting the deadline. This logic will be implemented in 
     * period check
     */
    TaskInfo_t task_info = GetTaskInfo(task_id);
    bool found = false;
    SLAType_t level = task_info.required_sla;
    unsigned int current = 0;

    bool must_be_gpu_compatible = true;

    // first time through, gpu compatibility must match. Second time through, not necessary
    for (unsigned gpu_run = 0; gpu_run < 2; gpu_run++) {

      if (found) break;
      //iterate through the different performance buckets
      for (auto& [mips, machines] : machines_by_mips) {
          unsigned int index = performance_indexRR[mips];
          //Scan each machine in the current performance tier
          for (int i = 0; i < machines.size(); i++) {
              MachineInfo_t machine = Machine_GetInfo(machines.at(index));
              //check to make sure CPU and GPU requirements are the same, for first time, not necessary second time through
              bool gpu_req_match = (gpu_run == 0 && machine.gpus == task_info.gpu_capable) || (gpu_run == 1);
              if (machine.cpu == task_info.required_cpu && gpu_req_match && machine.memory_size - machine.memory_used >= task_info.required_memory + 8) {
                  //check each VM inside this machine for space
                  //THIS IS JUST COPIED FROM RR CODE
                  vector<VMId_t>& vm_candidates = moreMachineInfo[index].active_vms;
                  VMId_t toAdd = -1;
                  for (const auto& VM : vm_candidates) {
                      VMInfo_t vm_info = VM_GetInfo(VMId_t(VM));
                      if (task_info.required_vm == vm_info.vm_type && task_info.required_cpu == vm_info.cpu) {
                          toAdd = VM;
                          break;
                      }
                  }
                  // if no useable VM, create one on this machine
                  if (toAdd == -1) {
                      toAdd = VM_Create(VMType_t(task_info.required_vm), task_info.required_cpu);
                      VM_Attach(toAdd, machine.machine_id);
                      vm_candidates.push_back(toAdd);
                  }
                  found = true;
                  if (level == SLA0) {
                      VM_AddTask(toAdd, task_id, HIGH_PRIORITY);
                  } else if (level == SLA2 || level == SLA1) {
                      VM_AddTask(toAdd, task_id, MID_PRIORITY);
                  } else {
                      VM_AddTask(toAdd, task_id, LOW_PRIORITY);
                  }
                  //update next index for RR
                  performance_indexRR[mips] = (index + i + 1) % machines.size();
              }
              index = (index + 1) % machines.size();
              if (found) 
                  break;
          }
          if (found)
              break;
      }
    }



}

void Scheduler::PeriodicCheck(Time_t now) {
  // This method should be called from SchedulerCheck()
  // SchedulerCheck is called periodically by the simulator to allow you to
  // monitor, make decisions, adjustments, etc. Unlike the other invocations of
  // the scheduler, this one doesn't report any specific event Recommendation:
  // Take advantage of this function to do some monitoring and adjustments as
  // necessary
  /**
   * This method is called every 60000 "ticks"
   * If we do not want to update the migration list that many times
   * we can just mod the total time by like 120000 for half the times etc.
   */


   /**
    * local_copy is used to maintain the list of vms that need to be migrated within the current periodic check
    */
    vector<VMId_t> local_copy;
    //This is an arbitrary number and is subject to change
    // TODO - Unsure of how to deal with math surrounding threshold and WHEN to migrate the VM based on tasks expected deadline, and its current trajectory
    Time_t threshold = 1000;
    //scan through every task to see what is about to be in SLA violation
    for (auto& machine : machines) {
        MachineInfo_t machine_info = Machine_GetInfo(machine);
        vector<VMId_t>& vm_candidates = moreMachineInfo[machine].active_vms;
        for (VMId_t VM : vm_candidates) {
            VMInfo_t vm_info = VM_GetInfo(VM);
            //handle a periodic cleanup so Dead VMs don't pile up
            if (vm_info.active_tasks.empty()) {
                VM_Shutdown(VM);
                vm_candidates.erase(std::remove(vm_candidates.begin(), vm_candidates.end(), VM), vm_candidates.end());
                continue;
            }
            //Assuming the VM isn't dead, we will check all of its tasks
            bool migrate = false;
            unsigned int vm_memory = 8; // get the memory of the vm + tasks_required memory
            for (TaskId_t task : vm_info.active_tasks) {
              TaskInfo_t task_info = GetTaskInfo(task);
                
                //TODO Not sure what the frick is going on here in terms of math and when to migrate
                unsigned int mips = machine_info.performance[machine_info.p_state];
                unsigned int instructions_left = GetTaskInfo(task).remaining_instructions;
                Time_t time_til_projected_completion = instructions_left / mips;
                Time_t projected_deadline = now + time_til_projected_completion;

                // we ensure task finishes within 99% of it's provided task time
                double size_of_red_zone = 0.01;
                Time_t red_zone_deadline = task_info.arrival + (1 - size_of_red_zone) * (task_info.target_completion - task_info.arrival);

                //Don't want to touch the GPU tasks because they are a pain to deal with
                //also set the priority to high because we want the tasks closer to deadline to finish quicker
                if (!GetTaskInfo(task).gpu_capable && projected_deadline > red_zone_deadline) { // TODO Dunno what conditions to check when migrating
                    SetTaskPriority(task, HIGH_PRIORITY);
                    migrate = true;
                }
                //TODO
                
                //this is just summing up the total memory of vm
                vm_memory += GetTaskInfo(task).required_memory;
                
            }
            //add the Vm to be migrated to a more powerful machine
            //simultaneously delete the machine from active vms
            if (migrate) {
                local_copy.push_back(VM);
                toMigrate[VM] += vm_memory;
                vm_candidates.erase(std::remove(vm_candidates.begin(), vm_candidates.end(), VM), vm_candidates.end());
            }
        }
    }

    /**
     * Once we finished scanning through all the tasks to figure out what VMs
     * we need to focus on, we then deal with actually migrating all the VMs
     * to higher power machines
     */

     //migrate all the VMs that are on local copy to the highest powered machine level
     //in a RR format
    while (!local_copy.empty()) {
        VMId_t aboutToMigrate = local_copy.back();
        VMInfo_t vm_info = VM_GetInfo(aboutToMigrate);
        local_copy.pop_back();
        bool found = false;
        //Find a suitable machine
        for (auto it = machines_by_mips.rbegin(); it != machines_by_mips.rend(); ++it) {
            auto& [mips, machines] = *it;
            unsigned int index = performance_indexRR[mips];
            for (int i = 0; i < machines.size(); i++) {
                //Scan each machine in this performance tier
                MachineInfo_t machine = Machine_GetInfo(machines.at(index));
                
                // also check to make sure we have enough space including the VMs that are currently migrating and are waiting to be put on machine
                unsigned pendingsum = 0;
                if (!pendingMigration[machine.machine_id].empty()) {
                    //sum all of the VMs waiting to be migrated
                    for (VMId_t VM : pendingMigration[machine.machine_id]) {
                        pendingsum += toMigrate[VM];
                    }
                }

                //check to make sure CPU and GPU requirements are the same
                if (machine.cpu == vm_info.cpu && machine.memory_size - machine.memory_used - pendingsum >= toMigrate[aboutToMigrate]) {
                    cout << "Machine: " << machine.machine_id << " Free memory " <<  machine.memory_size - machine.memory_used << "Memory to be used: " <<toMigrate[aboutToMigrate]<< endl;
                    //check each VM inside this machine for space
                    //THIS IS JUST COPIED FROM RR CODE
                    cout << "pending sum is: " << pendingsum << " for machine: " << machine.machine_id << endl;
                    VM_Migrate(aboutToMigrate, machine.machine_id);
                    pendingMigration[machine.machine_id].push_back(aboutToMigrate);
                    performance_indexRR[mips] = (index + i + 1) % machines.size();
                    found = true;
                    break;
                }
                index = (index + 1) % machines.size();
            }
            if (found)
                break;
        }
    }



}

void Scheduler::Shutdown(Time_t time) {
  // Do your final reporting and bookkeeping here.
  // Report about the total energy consumed
  // Report about the SLA compliance
  // Shutdown everything to be tidy :-)
  for (auto& vm : vms) {
    // while (VM_GetInfo(vm).active_tasks.size() != 0) {
    //   std::this_thread::sleep_for(std::chrono::milliseconds(100));  
    // }
    // // TaskInfo_t taskInfo = GetTaskInfo(VM_GetInfo(vm).active_tasks[0]);
    // // if (taskInfo.task_id == 81) {
    // //   std::cout << "task " << taskInfo.task_id << ": " << taskInfo.completed << std::endl;
    // // }
    
    VM_Shutdown(vm);
  }
  SimOutput("SimulationComplete(): Finished!", 4);
  SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
  // Do any bookkeeping necessary for the data structures
  // Decide if a machine is to be turned off, slowed down, or VMs to be migrated
  // according to your policy This is an opportunity to make any adjustments to
  // optimize performance/energy
  SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) +
                " is complete at " + to_string(now),
            4);

    //lets just say every 100 tasks we go through ALL machines and just cleanup
    //any VMs that have 0 tasks running on them
    //TODO This doesn't work for some reason about AddTask() shenangins as well
    // if (task_id % 100 == 0) {
    //     for (auto& machine : machines) {
    //         MachineInfo_t machine_info = Machine_GetInfo(machine);
    //         vector<VMId_t>& vm_candidates = moreMachineInfo[machine].active_vms;
    //         for (VMId_t VM : moreMachineInfo[machine].active_vms) {
    //             VMInfo_t vm_info = VM_GetInfo(VM);
    //             if (vm_info.active_tasks.size() == 0) {
    //                 VM_Shutdown(VM);
    //             }
    //         }
    //     }
    // }
}

// Public interface below

static Scheduler Scheduler;

void InitScheduler() {
  SimOutput("InitScheduler(): Initializing scheduler", 4);
  Scheduler.Init();
}

void HandleNewTask(Time_t time, TaskId_t task_id) {
  SimOutput("HandleNewTask(): Received new task " + to_string(task_id) +
                " at time " + to_string(time),
            4);
  Scheduler.NewTask(time, task_id);
}

void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
  SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) +
                " completed at time " + to_string(time),
            4);
  Scheduler.TaskComplete(time, task_id);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
  // The simulator is alerting you that machine identified by machine_id is
  // overcommitted
  SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) +
                " was detected at time " + to_string(time),
            0);
    MachineInfo_t machine = Machine_GetInfo(machine_id);
    cout << "memory used is: " << machine.memory_used << " with memory size: " << machine.memory_size << endl << " with active vm size: " << machine.active_vms;
    cout << "the active vms are: ";
    for (VMId_t VM : moreMachineInfo[machine_id].active_vms) {
        cout << VM << " ";
    }
}

void MigrationDone(Time_t time, VMId_t vm_id) {
  // The function is called on to alert you that migration is complete
  SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) +
                " was completed at time " + to_string(time),
            4);
  Scheduler.MigrationComplete(time, vm_id);
  migrating = false;
}

void SchedulerCheck(Time_t time) {
  // This function is called periodically by the simulator, no specific event
  SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time),
            4);
  Scheduler.PeriodicCheck(time);
//   static unsigned counts = 0;
//   counts++;
//   if (counts == 10) {
//     migrating = true;
//     VM_Migrate(1, 9);
//   }
}

void SimulationComplete(Time_t time) {
  // This function is called before the simulation terminates Add whatever you
  // feel like.
  cout << "SLA violation report" << endl;
  cout << "SLA0 : " << GetSLAReport(SLA0) << "%" << endl;
  cout << "SLA1 : " << GetSLAReport(SLA1) << "%" << endl; 
  cout << "SLA2 : " << GetSLAReport(SLA2) << "%"
       << endl;  // SLA3 do not have SLA violation issues
  cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
  cout << "Simulation run finished in " << double(time) / 1000000 << " seconds"
       << endl;
  SimOutput(
      "SimulationComplete(): Simulation finished at time " + to_string(time),
      4);

  Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
  // Called in response to an earlier request to change the state of a machine
}
