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




// typedef struct {
//   // bool completed;

//   // uint64_t total_instructions;
//   // uint64_t remaining_instructions;
//   // Time_t arrival;
//   // Time_t completion;
//   // Time_t target_completion;

//   Priority_t priority;

//   CPUType_t required_cpu;
//   unsigned required_memory;
//   VMType_t required_vm;

//   SLAType_t required_sla;
//   bool gpu_capable;

// } TaskCombo_t;


static bool migrating = false;
static unsigned active_machines;
static unsigned total_machines;
static unsigned total_tasks;
static CPUPerformance_t default_cpu_performance_state;


// tbd

static std::vector<bool> vm_is_migrating; // track if a vm is current migrating to a diff machine
static std::vector<bool> machine_is_changing_state; // track if a machine is in the process of changing state
static std::unordered_map<unsigned int, pair<unsigned int, vector<VMId_t>>> taskCombos; // <task combo, <total instrs, list of possible VMs>>
static std::unordered_map<MachineId_t, vector<VMId_t>> machine_to_active_vms;
void Scheduler::Init() {
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





  SimOutput("Scheduler::Init(): Total number of machines is " +
    to_string(Machine_GetTotal()),
    3);
  SimOutput("Scheduler::Init(): Initializing scheduler", 1);


  // here, we update each machines mips, which we'll use to determine how many vms of each type to assign
  for (unsigned i = 0; i < total_machines; i++) {
    MachineInfo_t machine_info_i = Machine_GetInfo(MachineId_t(i));
    machines.push_back(MachineId_t(i));
  }






  // 1. obtain info on task combos
  for (unsigned i = 0; i < total_tasks; i++) {
    TaskInfo_t taskInfo = GetTaskInfo(TaskId_t(i));

    unsigned int taskcombo = taskInfo.required_cpu * 10 + taskInfo.required_vm;
    taskCombos[taskcombo].first++;
    // std::get<1>(taskCombos[taskcombo]) += 1; // if u use a tuple
  }



  // 2 based on taskCombos, create <cpu, vm> 
  vector<vector<unsigned int>> machine_to_vm (4); // <machine xyz, all necessary vms>
  for (const auto& pair : taskCombos) {
    unsigned int taskcombo = pair.first;
    unsigned int taskcombo_cpu = taskcombo / 10;
    unsigned int taskcombo_vm = taskcombo % 10;

    machine_to_vm[taskcombo_cpu].push_back(taskcombo_vm);
  }



  // 3. for every machine, give it all possible VMS
  for (unsigned i = 0; i < Machine_GetTotal(); i++) {
    MachineInfo_t machineInfo = Machine_GetInfo(MachineId_t(i));

    vector<unsigned int>& candidate_vms = machine_to_vm[machineInfo.cpu];
    for (unsigned j = 0; j < candidate_vms.size(); j++) {
      VMId_t vm_id = VM_Create(VMType_t(j), CPUType_t(machineInfo.cpu)); 
      vms.push_back(vm_id);
      VM_Attach(vm_id, (MachineId_t)i);
      machine_to_active_vms[i].push_back(vm_id);
    }
  }



  // PrintStuff();



  
  // list() of lists
  // list<list<taskid_t>>
  // out list: required resources
      // tasks that requiure that resource
  
      // List 1: resource x (e.g. x86, ARM)
      //    The list of tasks that require this resource
      //List 2: resource y
      //    the list of tasks the require resource y

  // <CPU, list<possible machines>>
  // <resource xzy, list<tasks>



  // for (unsigned i = 0; i < total_machines; i++) {
  //   for (unsigned j = 0; j < Machine_GetInfo(MachineId_t(i)).num_cpus; j++) {
  //     VM_Attach(vms[i], machines[i]);
  //   }
  // }



  // set s-state + p-state

  // do you want it to be dynamic in some way? idk yet, dakshin said worth a try
  // to manipulate performance (p3 vs p0) but he only messed with s-state
  // bool dynamic = true;
  // if (dynamic)
    // for (unsigned i = 0; i < 4; i++) {
    //   Machine_SetState(MachineId_t(i), MachineState_t(S0));

    //   for (unsigned j = 0; j < 8; j++) {
    //     Machine_SetCorePerformance(MachineId_t(i), j, default_cpu_performance_state);
    //   }

    // }

  // PrintStuff();


  SimOutput("Scheduler::Init(): VM ids are " + to_string(vms[0]) + " ahd " +
    to_string(vms[1]),
    3);

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

  //Greedy algorithm based on memory
  // Find the correct resources for the task
  TaskInfo_t task_info = GetTaskInfo(task_id);
  Priority_t priority = (task_info.required_sla == SLA0 || task_info.required_sla == SLA1) ? HIGH_PRIORITY : LOW_PRIORITY;
  //get the correct resource to machine "vector"

  bool added_task = false;
  for (unsigned i = 0; i < Machine_GetTotal(); i++) {
    MachineInfo_t M_info = Machine_GetInfo(i);

    if (added_task) break;
    vector<VMId_t>& candidate_vms = machine_to_active_vms[i];
    for (unsigned j = 0; j < candidate_vms.size(); j++) {
      if (task_info.required_vm == VM_GetInfo(candidate_vms[j]).vm_type && task_info.required_cpu == M_info.cpu && M_info.memory_size - M_info.memory_used >= task_info.required_memory + 8) {
        VM_AddTask(machine_to_active_vms[i][j], task_id, priority);
        added_task = true;
        break;
      }
    }

  }

  if (task_id % 10000 == 0) cout << "started task=" << task_id << endl;
  
  
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

    if (task_id % 10000 == 0) cout << "ended task=" << task_id << endl;

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
