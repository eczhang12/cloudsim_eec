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


static bool migrating = false;
static unsigned active_machines = 16;
static unsigned machines_in_use = 1;

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

  
  
  // create your VMs and attach them to hardware machines.
  // init state of VM and machine is to fully turn on everything, setting every state to 0
  for (unsigned i = 0; i < active_machines; i++)
  vms.push_back(VM_Create(LINUX, X86));
  for (unsigned i = 0; i < active_machines; i++) {
    machines.push_back(MachineId_t(i));
  }
  for (unsigned i = 0; i < active_machines; i++) {
    VM_Attach(vms[i], machines[i]);
  }

  bool dynamic = true;
  if (dynamic)
    for (unsigned i = 0; i < 4; i++)
      for (unsigned j = 0; j < 8; j++)
        Machine_SetCorePerformance(MachineId_t(0), j, P3);

  // Turn off the ARM machines - see input file
  for (unsigned i = 24; i < Machine_GetTotal(); i++) {
    Machine_SetState(MachineId_t(i), S5);
    // std::cout << "turning off arm machine " << i << std::endl;
  }


  // MachineInfo_t machineInfo = Machine_GetInfo(machines[0]);
  // std::cout << "current s state: " << machineInfo.s_state << std::endl;
  // std::cout << "current p state: " << machineInfo.p_state << std::endl;
  
  // std::cout << "machine available memory: " << machineInfo.memory_size - machineInfo.memory_used << " / " << machineInfo.memory_size << std::endl;


  SimOutput("Scheduler::Init(): VM ids are " + to_string(vms[0]) + " ahd " +
                to_string(vms[1]),
            3);

}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
  // Update your data structure. The VM now can receive new tasks
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

  if (migrating) {
    VM_AddTask(vms[0], task_id, GetTaskInfo(task_id).priority);

  } else {
    // MachineInfo_t machineInfo = Machine_GetInfo(machines[task_id % active_machines]);
    // std::cout << "machine " << task_id % active_machines << " used memory before adding task " << task_id << ": " << machineInfo.memory_used << " / " << machineInfo.memory_size << std::endl;



  // approach no. 2: greedy
  // assign a task to a vm if it has enough memory for the task
    bool addedTask = false;
    MachineInfo_t machineInfo;
    for (signed i = 0; i < machines_in_use; i++) {
    // for (signed i = machines_in_use - 1; i >= 0; i--) {
      machineInfo = Machine_GetInfo(MachineId_t(machines[i]));
      unsigned memory_remaining = machineInfo.memory_size - machineInfo.memory_used;
      if (memory_remaining >= GetTaskMemory(task_id) && memory_remaining >= machineInfo.memory_size / 2) {
        std::cout << "machine " << i << " used memory before adding task " << task_id << ": " << machineInfo.memory_used << " / " << machineInfo.memory_size << std::endl;
        VM_AddTask(vms[i], task_id, GetTaskInfo(task_id).priority);
        addedTask = true;
        std::cout << "machine " << i << " used memory after adding task " << task_id << ": " << machineInfo.memory_used << " / " << machineInfo.memory_size << std::endl;
      }
    }

    if (!addedTask) {
      machines_in_use++;
      std::cout << "new machine " << machines_in_use - 1 << " used memory before adding task " << task_id << ": " << machineInfo.memory_used << " / " << machineInfo.memory_size << std::endl;

      VM_AddTask(vms[machines_in_use - 1], task_id, GetTaskInfo(task_id).priority);
      std::cout << "new machine " << machines_in_use - 1 << " used memory after adding task " << task_id << ": " << machineInfo.memory_used << " / " << machineInfo.memory_size << std::endl;
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
  static unsigned counts = 0;
  counts++;
  if (counts == 10) {
    migrating = true;
    VM_Migrate(1, 9);
  }
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
