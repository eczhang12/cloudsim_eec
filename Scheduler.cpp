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
  // bool completed;

  // uint64_t total_instructions;
  // uint64_t remaining_instructions;
  // Time_t arrival;
  // Time_t completion;
  // Time_t target_completion;

  // Priority_t priority;

  // CPUType_t required_cpu;
  // unsigned required_memory;
  // VMType_t required_vm;

  // SLAType_t required_sla;
  // bool gpu_capable;

  unsigned total_tasks; // # of tasks of this combo
  uint64_t total_instructions; // # of total instructions for this combo
  
  vector<MachineId_t> gpu_machines;
  vector<MachineId_t> nongpu_machines;

  unsigned int gpu_machines_index;
  unsigned int nongpu_machines_index;

} TaskComboInfo_t;


// global attributes 
static bool migrating = false;
static unsigned active_machines;
static unsigned total_machines;
static unsigned total_tasks;
static CPUPerformance_t default_cpu_performance_state;
static bool sim_has_gpus;


// machine/vm specific attributes
static std::vector<unsigned int> machine_mips;
static std::vector<bool> vm_is_migrating; // track if a vm is current migrating to a diff machine
static std::vector<bool> machine_is_changing_state; // track if a machine is in the process of changing state
static std::unordered_map<unsigned int, TaskComboInfo_t> taskCombos; // <task combo, <total instrs, list of possible VMs>>
static std::vector<std::vector<VMId_t>> machine_to_active_vms;

void Scheduler::PrintStuff() {

  // 
  // printing task info
  // 

  // for (unsigned i = 0; i < total_tasks; i++) {
  //   TaskInfo_t task_info_i = GetTaskInfo(TaskId_t(i));
  //   cout << " task " << i << ": ";
  //   cout << " start=" << task_info_i.arrival/1000;
  //   cout << " req vm=" << task_info_i.required_vm;
  //   cout << " req cpu=" << task_info_i.required_cpu;
  //   cout << " req mem=" << task_info_i.required_memory;
  //   cout << " req sla=" << task_info_i.required_sla;
  //   cout << " instructions=" << task_info_i.total_instructions/1000000;
  //   cout << endl;
  // }

  // 
  // printing machine info
  // 

  // for (unsigned i = 0; i < Machine_GetTotal(); i++) {
  //   MachineInfo_t machine_info_i = Machine_GetInfo(MachineId_t(i));
  //   cout << " machine " << i << ": ";
  //   cout << " cpus(" << machine_info_i.num_cpus << ")=" << to_string(machine_info_i.cpu);
  //   cout << " memory=" << machine_info_i.memory_size;
  //   cout << " gpu?=" << machine_info_i.gpus;
  //   cout << " mips=" << machine_mips[i];
  //   cout << endl;
  // }


  // 
  // printing vm info
  // 

  // for (unsigned i = 0; i < total_machines; i++) {
  //   VMInfo_t vm_info_i = VM_GetInfo(VMId_t(i));
  //   cout << " vm " << i << ": ";
  //   cout << " active tasks=" << vm_info_i.active_tasks.size();
  //   cout << " type=" << to_string(vm_info_i.vm_type);
  //   cout << " cpu=" << to_string(vm_info_i.cpu);
  //   cout << endl;
  // }


    // 
  // print taskcombo tasks per combo
  // 

//   for (const auto& pair : taskCombos) {
//     unsigned int taskcombo = pair.first;
//     unsigned int taskcombo_cpu = taskcombo / 10;
//     unsigned int taskcombo_vm = taskcombo % 10;
//     unsigned int tasks_of_combo = pair.second.first;

//     cout << to_string((CPUType_t) taskcombo_cpu) << " " << to_string((VMType_t) taskcombo_vm) << " :" << tasks_of_combo << endl;
// }

  // 
  // print vms for each taskcombo
  // 

  // for (const auto& pair : taskCombos) {
  //   unsigned int taskcombo = pair.first;
  //   unsigned int taskcombo_cpu = taskcombo / 10;
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
  default_cpu_performance_state = P3;
  total_tasks = GetNumTasks();
  sim_has_gpus = false;



  cout << "xyz1" << endl;

  SimOutput("Scheduler::Init(): Total number of machines is " +
    to_string(Machine_GetTotal()),
    3);
  SimOutput("Scheduler::Init(): Initializing scheduler", 1);

  

  // here, we update each machines info, which we'll use to determine how many vms of each type to assign
  for (unsigned i = 0; i < total_machines; i++) {
    MachineInfo_t machine_info_i = Machine_GetInfo(MachineId_t(i));
    machines.push_back(MachineId_t(i));
    
    unsigned int n_cpus = machine_info_i.num_cpus;
    
    unsigned int mips = machine_info_i.performance[default_cpu_performance_state];
    machine_mips.push_back((unsigned int)(n_cpus * mips));

    sim_has_gpus = sim_has_gpus || machine_info_i.gpus;

    machine_to_active_vms.push_back(std::vector<VMId_t>(0));
  }

  for (unsigned i = 0 ; i < total_machines; i++) {
    for (unsigned j = 0; j < Machine_GetInfo(MachineId_t(i)).num_cpus; j++) {
      Machine_SetCorePerformance(MachineId_t(i), j, CPUPerformance_t(default_cpu_performance_state));
    }
  }

  cout << "xyz2" << endl;

  // gpu compatible tasks should only use gpu enabled machines?

  // for (taskCombo : all taskCombos) {
    // for (machine : all machines) {
      // 
    // }
  // }


  // 1. obtain info on task combos, initialize the object
  for (unsigned i = 0; i < total_tasks; i++) {
    TaskInfo_t taskInfo = GetTaskInfo(TaskId_t(i));

    unsigned int taskcombo = taskInfo.gpu_capable * 100 + taskInfo.required_cpu * 10 + taskInfo.required_vm;
    taskCombos[taskcombo].total_tasks += 1;
    taskCombos[taskcombo].total_instructions += taskInfo.total_instructions;
    
  }

  cout << "xyz3" << endl;


  
  
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
   
    CPUType_t taskcombo_cpu = CPUType_t(taskcombo / 10);
    VMType_t taskcombo_vm = VMType_t(taskcombo % 10);
    
    cout << "taskcombo(" << taskcombo << "): gpu_machines=" << taskCombos[taskcombo].gpu_machines.size() << " nongpu_machines=" << taskCombos[taskcombo].nongpu_machines.size() << endl;
  }

  // 3

  
  
  
  cout << "xyz4" << endl;
  
  

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

  cout << "started task: " << task_id << endl;
  TaskInfo_t task_info = GetTaskInfo(task_id);
  //get the correct resource to machine "vector"

  TaskComboInfo_t taskComboInfo = taskCombos[task_info.gpu_capable * 100 + task_info.required_cpu * 10 + task_info.required_vm];
  // cout << "taskComboInfo: " << taskComboInfo << endl;
  //We have the taskCombo, we should find which GPU or NONGPU list of machines to use
  vector<MachineId_t> machine_candidates = (task_info.gpu_capable) ? taskComboInfo.gpu_machines : taskComboInfo.nongpu_machines;
  unsigned index = (task_info.gpu_capable) ? taskComboInfo.gpu_machines_index : taskComboInfo.nongpu_machines_index;
  

  //cycle through every machine related to the resource to look for available vm
  while (true) {
    MachineInfo_t machine = Machine_GetInfo(machine_candidates[index]);

    //check that this machine has enough memory first
    if (machine.memory_size - machine.memory_used >= task_info.required_memory + 16) {
      //Scan through the VMs to make sure it has the VM we require
      vector<VMId_t> vm_candidates = machine_to_active_vms[machine.machine_id];
      VMId_t toAdd = -1;
      for (const auto& VM : vm_candidates) {
        VMInfo_t vm_info = VM_GetInfo(VMId_t(VM));
        if (task_info.required_vm == vm_info.vm_type)
          toAdd = VM;
      }
      //we did not find it so we must create the VM
      if (toAdd == -1) {      

        toAdd = VM_Create(VMType_t(task_info.required_vm), task_info.required_cpu);
        VM_Attach(toAdd, machine.machine_id);

      }
      //now we have a VM to add the task to
      VM_AddTask(toAdd, task_id, LOW_PRIORITY);

      break;
    }
    index++;
    index = index % machine_candidates.size();
  }
  if (task_info.gpu_capable == true)
    taskComboInfo.gpu_machines_index = index;
  else
    taskComboInfo.nongpu_machines_index = index;

}
    // Skeleton code, you need to change it according to your algorithm




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
  if (unsigned(taskinfo.target_completion) < unsigned (now)) {
    unsigned combo = 100 * taskinfo.gpu_capable + 10 * taskinfo.required_cpu + taskinfo.required_vm;
    cout << "failed task id=" << task_id << ", combo=" << combo << endl;
  }
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
