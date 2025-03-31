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




typedef struct {
  unsigned total_tasks; // # of tasks of this combo
  uint64_t total_instructions; // # of total instructions for this combo
  vector<MachineId_t> gpu_machines; // gpu_machines available for this taskCombo
  vector<MachineId_t> nongpu_machines; // nongpu_machines available for this taskCombo
  unsigned int gpu_machines_index; // used for round robin
  unsigned int nongpu_machines_index; // used for round robin

} TaskComboInfo_t;



typedef struct {
  vector<VMId_t> active_vms;
  vector<unsigned int> total_mips;

  bool changing_pstate; // is machine changing performance state
  bool changing_cstate; // is machine changing power state
} MoreMachineInfo_t;



typedef struct {
  VMId_t vm_ran_on;
  MachineId_t machine_ran_on;
  Time_t finish_time;
} MoreTaskInfo_t;


// global attributes 
static bool migrating = false;

static unsigned total_machines;
static unsigned total_tasks;
static CPUPerformance_t def_cpu_pstate;



// machine/vm specific attributes
static std::unordered_map<unsigned int, TaskComboInfo_t> taskCombos; // <task combo, <total instrs, list of possible VMs>>
static std::unordered_map<MachineId_t, MoreMachineInfo_t> moreMachineInfo;
static std::unordered_map<TaskId_t, MoreTaskInfo_t> moreTaskInfo;

static std::vector<bool> vm_is_migrating; // track if a vm is current migrating to a diff machine

void Scheduler::PrintTaskInfo(TaskId_t task_id) {
  // 
  // printing task info
  // 
  

    TaskInfo_t task_info_i = GetTaskInfo(TaskId_t(task_id));
    cout << " -----INFORMATION about task " << task_id << ": ";
    cout << " start=" << task_info_i.arrival;
    cout << " taskCombo=" << task_info_i.gpu_capable * 100 + task_info_i.required_cpu * 10 + task_info_i.required_vm;
    cout << " req vm=" << task_info_i.required_vm;
    cout << " req cpu=" << task_info_i.required_cpu;
    cout << " req mem=" << task_info_i.required_memory;
    cout << " req sla=" << task_info_i.required_sla;
    cout << " instructions=" << task_info_i.total_instructions;
    cout << endl;

}


void Scheduler::PrintVMInfo(VMId_t vm_id) {

  
    // 
    // printing vm info
    // 
  
    for (unsigned i = 0; i < total_machines; i++) {
      VMInfo_t vm_info_i = VM_GetInfo(VMId_t(i));
      cout << " -----INFORMATION about vm " << i << ": ";
      cout << " active tasks=" << vm_info_i.active_tasks.size();
      cout << " type=" << to_string(vm_info_i.vm_type);
      cout << " cpu=" << to_string(vm_info_i.cpu);
      cout << endl;
    }
}

void Scheduler::PrintMachineInfo(MachineId_t machine_id) {
  // 
  // printing machine info
  // 
  

  MachineInfo_t m_info = Machine_GetInfo(MachineId_t(machine_id));
  cout << " -----INFRMATION about machine " << machine_id << ": ";
  cout << " cpus(" << m_info.num_cpus << ")=" << to_string(m_info.cpu);
  cout << " memory=" << m_info.memory_size;
  cout << " gpu?=" << m_info.gpus;
  cout << " vms=[";
  for (unsigned j = 0; j < moreMachineInfo[machine_id].active_vms.size(); j++)
  {
    cout << moreMachineInfo[machine_id].active_vms[j] << ",";
  }
  cout << "]";
  cout << endl;
}




void Scheduler::PrintStuff() {





    // 
  // print taskcombo tasks per combo
  // 

//   for (const auto& pair : taskCombos) {
//     unsigned int taskcombo = pair.first;
//     unsigned int taskcombo_cpu = (taskcombo / 10) % 10;
//     unsigned int taskcombo_vm = taskcombo % 10;
//     unsigned int tasks_of_combo = pair.second.first;

//     cout << to_string((CPUType_t) taskcombo_cpu) << " " << to_string((VMType_t) taskcombo_vm) << " :" << tasks_of_combo << endl;
// }

  // 
  // print vms for each taskcombo
  // 

  // for (const auto& pair : taskCombos) {
  //   unsigned int taskcombo = pair.first;
  //   unsigned int taskcombo_cpu = (taskcombo / 10) % 10;
  //   unsigned int taskcombo_vm = taskcombo % 10;

  //   cout << "taskcombo: cpu("<< taskcombo_cpu <<") vm("<< taskcombo_vm<<") avail vms:";

  //   for (unsigned i = 0; i < taskCombos[taskcombo].second.size(); i++) {
  //     cout << taskCombos[taskcombo].second[i] << " ";
  //   }
  //   cout << endl;
  // }


  // for (unsigned i = 0; i < vms.size(); i++) {
  //   cout << "vm " << i << ": " << VM_GetInfo(vms[i]).vm_type << endl;
  // }


}


void Scheduler::Init() {



  cout << "begin init function" << endl;
  // Find the parameters of the clusters
  // Get the total number of machines
  // For each machine:
  //      Get the type of the machine
  //      Get the memory of the machine
  //      Get the number of CPUs
  //      Get if there is a GPU or not

  // match tasks based on the following: 
    // for machines:

      // gpu_enabled
      // max_memory and memory_in_use
      // cpu_type - CPUType_t
      // 
      // new datatype: total_instructions for machine?

    // for tasks:

      // gpu_enabled?
      // required_memory
      // required_vm - VMType_t
      // required_cpu - CPUType_t
      // required_sla - required_sla
      // priority







  // vars
  total_machines = Machine_GetTotal();
  def_cpu_pstate = P0;
  total_tasks = GetNumTasks();



  cout << "xyz1" << endl;

  SimOutput("Scheduler::Init(): Total number of machines is " +
    to_string(Machine_GetTotal()),
    3);
  SimOutput("Scheduler::Init(): Initializing scheduler", 1);



  // update moreM_info
  for (unsigned i = 0; i < total_machines; i++) {
    MachineInfo_t M_info = Machine_GetInfo(MachineId_t(i));
    MoreMachineInfo_t& moreM_info = moreMachineInfo[MachineId_t(i)];
    machines.push_back(MachineId_t(i));

    // update total_mips
    unsigned int n_cpus = M_info.num_cpus;
    for (unsigned j = 0; j < 4; j++) {
      moreM_info.total_mips.push_back(unsigned(n_cpus * M_info.performance[j]));
    }

    // this machine isn't changing state
    moreM_info.changing_cstate = false;
    moreM_info.changing_pstate = false;

    // set core performance
    for (unsigned j = 0; j < Machine_GetInfo(MachineId_t(i)).num_cpus; j++) {
      Machine_SetCorePerformance(MachineId_t(i), j, CPUPerformance_t(def_cpu_pstate));
    }
  }





  // 1. obtain info on task combos, initialize the object
  for (unsigned i = 0; i < total_tasks; i++) {
    TaskInfo_t taskInfo = GetTaskInfo(TaskId_t(i));

    unsigned int taskcombo = taskInfo.gpu_capable * 100 + taskInfo.required_cpu * 10 + taskInfo.required_vm;
    taskCombos[taskcombo].total_tasks += 1;
    taskCombos[taskcombo].total_instructions += taskInfo.total_instructions;

  }



  // 2 based on taskCombos, create <cpu + gpu + vm, all potential machines> combos

  for (const auto& pair : taskCombos) {
    unsigned int taskcombo = pair.first;
    cout << "tried combo: " << taskcombo << endl;

    int taskcombo_gpu = taskcombo / 100;
    CPUType_t taskcombo_cpu = CPUType_t((taskcombo / 10) % 10);
    VMType_t taskcombo_vm = VMType_t(taskcombo % 10);

    // repeat this process for every machine for every taskcombo
    for (unsigned i = 0; i < total_machines; i++) {
      MachineInfo_t machine_info = Machine_GetInfo(MachineId_t(i));
      bool machine_gpu = machine_info.gpus;
      CPUType_t machine_cpu = machine_info.cpu;
      if (machine_cpu == taskcombo_cpu) {
        // use task combo (cpu, gpu, and vm) as key
        if (machine_gpu) {
          taskCombos[taskcombo].gpu_machines.push_back(MachineId_t(i));
        }
        else {
          taskCombos[taskcombo].nongpu_machines.push_back(MachineId_t(i));
        }
      }
    }
  }

  for (const auto& pair : taskCombos) {
    unsigned int taskcombo = pair.first;

    bool taskcombo_gpu = taskcombo / 100;

    CPUType_t taskcombo_cpu = CPUType_t((taskcombo / 10) % 10);
    VMType_t taskcombo_vm = VMType_t(taskcombo % 10);

    cout << "taskcombo(" << taskcombo << "): gpu_machines=" << taskCombos[taskcombo].gpu_machines.size() << " nongpu_machines=" << taskCombos[taskcombo].nongpu_machines.size() << endl;
  }



  // 3 for every machine, add on VMs proportional to <cpu + gpu-> vm combos>
  // add in max # of vms. adding in more VMs doesn't cost more energy, only keeping entire machines on uses energy
  // unsigned machine_num = 0;
  // while (machine_num < total_machines) {
  //   MachineInfo_t machineInfo = Machine_GetInfo(MachineId_t(machine_num));
  //   unsigned cpu_gpu_combo = machineInfo.gpus * 100 + machineInfo.cpu * 10;

  //   CPUType_t cpuType = machineInfo.cpu;
  //   unsigned num_cpus = machineInfo.num_cpus;

  //   for (unsigned i = 0; i < num_cpus; i++) {
  //     machine_to_vm[]
  //   }
  //   vector<unsigned int> vm_pairs = machine_to_vm[cpuType];
  //   VMType_t vmType = (VMType_t) vm_pairs[machine_num % vm_pairs.size()];

  //   vms.push_back(VM_Create(vmType, cpuType));
  //   VM_Attach((VMId_t)machine_num, (MachineId_t)machine_num);

  //   // now that we have taskcombo (vm + cpu combo), we can add the current machine_num to our taskCombos
  //   unsigned taskcombo = 10 * cpuType + vmType;
  //   taskCombos[taskcombo].vm_candidates.push_back(machine_num);

  //   machine_num++;
  // }


  cout << "reached end of init function" << endl;

}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
  // Update your data structure. The VM now can receive new tasks
}


void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
  cout << "started adding task " << task_id;
  // Get the task parameters
  //  IsGPUCapable(task_id);
  //  GetMemory(task_id);
  //  RequiredVMType(task_id);
  //  RequiredSLA(task_id);
  //  RequiredCPUType(task_id);
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


  // -----round robin-----
  // search list of candidate_vms for a potential vm, keeping track of vm_index for RR
  // Find the correct resources for the task

  if (task_id == 20096) cout << "this is 20096" << endl;


  TaskInfo_t task_info = GetTaskInfo(task_id);
  //get the correct resource to machine "vector"
  
  TaskComboInfo_t& taskComboInfo = taskCombos[task_info.gpu_capable * 100 + task_info.required_cpu * 10 + task_info.required_vm];

  // cout << "taskComboInfo: " << taskComboInfo << endl;
  //We have the taskCombo, we should find which GPU or NONGPU list of machines to use

  if (task_id == 20096) cout << "this is 20096" << endl;
  if (task_id == 20096) PrintTaskInfo(task_id);


  vector<MachineId_t>& machine_candidates = (task_info.gpu_capable) ? taskComboInfo.gpu_machines : taskComboInfo.nongpu_machines;
  unsigned index = (task_info.gpu_capable) ? taskComboInfo.gpu_machines_index : taskComboInfo.nongpu_machines_index;
  unsigned indexholder = index;
  vector<MachineId_t>& backup_candidates = (task_info.gpu_capable) ? taskComboInfo.nongpu_machines : taskComboInfo.gpu_machines;
  unsigned backup_index = (task_info.gpu_capable) ? taskComboInfo.nongpu_machines_index : taskComboInfo.gpu_machines_index;

  if (task_id == 20096) cout << "made it here: all vars defined" << endl;
  bool found = false;
  //cycle through every candidate machine to find host vm
  for (unsigned iterations = 0; iterations < machine_candidates.size(); iterations++) {
    cout << "it=" << iterations << " machine used=" << machine_candidates[index] << " gpu-cap task?=" << task_info.gpu_capable << endl;
    MachineInfo_t machine = Machine_GetInfo(machine_candidates[index]);
    // cout << "idnex at beginning is: " << index;
    // if machine has enough memory
    if (machine.memory_size - machine.memory_used >= task_info.required_memory + 8) {
      // if (task_id == 20096) cout << "made it here: have enough memory, entered a machine" << endl;
      // find useable VM
      vector<VMId_t>& vm_candidates = moreMachineInfo[machine_candidates[index]].active_vms;
      VMId_t toAdd = -1;
      for (const auto vm : vm_candidates) {
        if (task_id == 20096) cout << "made it here: entered vm loop: vm_id=" << vm << endl;
        VMInfo_t vm_info = VM_GetInfo(VMId_t(vm));
        if (task_id == 20096) cout << "task_info.required_vm=" << task_info.required_vm << " vm_info.vm_type=" << vm_info.vm_type << endl;
        if (task_info.required_vm == vm_info.vm_type) {
          toAdd = vm;
          break;
        }
      }
      if (task_id == 20096) cout << "made it here: exitted vm loop, toAdd=" << toAdd << endl;
      // if no useable VM, create one on this machine
      if (toAdd == -1) {
        if (task_id == 20096) cout << "made it here: enter toAdd==-1" << endl;
        toAdd = VM_Create(VMType_t(task_info.required_vm), task_info.required_cpu);
        VM_Attach(toAdd, machine.machine_id);
        vm_candidates.push_back(toAdd);
        if (task_id == 20096) cout << "made it here: exit toAdd==-1" << endl;
      }


      //now we have a VM to add the task to
      VM_AddTask(toAdd, task_id, LOW_PRIORITY);
      if (task_id == 20096) cout << "made it here: xyz" << endl;
      index = (index + 1) % machine_candidates.size();
      if (task_id == 20096) cout << "made it here: xyzw" << endl;
      found = true;

      if (task_id == 20096) cout << "made it here: found is true" << endl;

      if (machine.gpus) {
        taskComboInfo.gpu_machines_index = index;
      } else {
        taskComboInfo.nongpu_machines_index = index;
      }

      // update moreTaskInfo data structure for loggin purposes in the future
      moreTaskInfo[task_id].machine_ran_on = machine.machine_id;
      moreTaskInfo[task_id].vm_ran_on = toAdd;

      // we found a vm and added a task to it. break
      break;
    }
    index = (index + 1) % machine_candidates.size();
  }


  //otherwise check the backup machine
  if (!found && backup_candidates.size() != 0) {
    cout << "We enter here" << endl;
    for (unsigned iterations = 0; iterations < backup_candidates.size(); iterations++) {
        cout << "backup index is: " << backup_index << endl;
        MachineInfo_t machine = Machine_GetInfo(backup_candidates[backup_index]);
        // cout << "idnex at beginning is: " << index;
        // if machine has enough memory
        if (machine.memory_size - machine.memory_used >= task_info.required_memory + 8) {
          // find useable VM
          vector<VMId_t>& vm_candidates = moreMachineInfo[backup_candidates[backup_index]].active_vms;
          VMId_t toAdd = -1;
          for (const auto& VM : vm_candidates) {
            VMInfo_t vm_info = VM_GetInfo(VMId_t(VM));
            if (task_info.required_vm == vm_info.vm_type) {
              toAdd = VM;
              break;
            }
          }
          // if no useable VM, create one on this machine
          if (toAdd == -1) {
            cout << "we did not find a suitable vm" << endl;
            toAdd = VM_Create(VMType_t(task_info.required_vm), task_info.required_cpu);
            VM_Attach(toAdd, machine.machine_id);
            vm_candidates.push_back(toAdd);
          }
          //now we have a VM to add the task to
          VM_AddTask(toAdd, task_id, LOW_PRIORITY);
          backup_index = (backup_index + 1) % backup_candidates.size();
          found = true;
    
          //update the index
          if (machine.gpus) {
            taskComboInfo.gpu_machines_index = backup_index;
          } else {
            taskComboInfo.nongpu_machines_index = backup_index;
          }

        //   if (machine.gpus && task_info.gpu_capable) {
        //     taskComboInfo.gpu_machines_index = index;
        //   }
        //   else if (machine.gpus && !task_info.gpu_capable || !machine.gpus && task_info.gpu_capable) {
        //     cout << "mixed"; // tbd the case where nongpu tasks can use gpu machines, not done yet
        //   }
        //   else {
        //     taskComboInfo.nongpu_machines_index = index;
        //   }
    
          // update moreTaskInfo data structure for loggin purposes in the future
          moreTaskInfo[task_id].machine_ran_on = machine.machine_id;
          moreTaskInfo[task_id].vm_ran_on = toAdd;
    
          // we found a vm and added a task to it. break
          break;
        }
        backup_index = (backup_index + 1) % backup_candidates.size();
      }
    
  }

  cout << "finished adding task " << task_id;
  cout << endl;
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






  for (unsigned i = 0; i < total_machines; i++) {

    cout << "machine " << i << ": ";

    for (unsigned j = 0; j < moreMachineInfo[MachineId_t(i)].active_vms.size(); j++) {
      cout << " " << moreMachineInfo[MachineId_t(i)].active_vms[j] << " ";
    }

    cout << endl;
  }



  for (auto& vm : vms) {
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

  TaskInfo_t taskinfo = GetTaskInfo(TaskId_t(task_id));
  if (unsigned(taskinfo.target_completion) < unsigned(now)) {
    unsigned combo = 100 * taskinfo.gpu_capable + 10 * taskinfo.required_cpu + taskinfo.required_vm;
    cout << "failed task id=" << task_id << ", combo=" << combo << ", machine=" << moreTaskInfo[task_id].machine_ran_on << ", vm=" << moreTaskInfo[task_id].vm_ran_on << ", sla" << GetTaskInfo(task_id).required_sla <<  endl;
  }

  // update our moreTaskInfo object for logging in the future
  moreTaskInfo[task_id].finish_time = now; 
  
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
  // static unsigned counts = 0;
  // counts++;
  // if (counts == 10) {
  //   migrating = true;
  //   VM_Migrate(1, 9);
  // }
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
