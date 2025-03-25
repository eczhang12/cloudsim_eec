//
//  Interfaces.h
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 11/3/24.
//

#ifndef Interfaces_h
#define Interfaces_h

// This header defines the public interfaces between the various modules. We have the following modules:
// Debugging (messages and exceptions)
// Machines
// Scheduler
// Tasks
// VM (virtual machines)

#include <string>
#include <stdexcept>

#include "SimTypes.h"

// Debugging Interface
extern void             SimOutput(string msg, unsigned verbose_level);
extern void             ThrowException(string err_msg);
extern void             ThrowException(string err_msg, string further_input);
extern void             ThrowException(string err_msg, unsigned further_input);

// Machine Interface
extern CPUType_t        Machine_GetCPUType(MachineId_t machine_id);
extern uint64_t         Machine_GetEnergy(MachineId_t machine_id);
extern double           Machine_GetClusterEnergy();
extern MachineInfo_t    Machine_GetInfo(MachineId_t machine_id);
extern unsigned         Machine_GetTotal();
extern void             Machine_SetCorePerformance(MachineId_t machine_id, unsigned core_id, CPUPerformance_t p_state);  // This is oriented toward dynamic energy
extern void             Machine_SetState(MachineId_t machine_id, MachineState_t s_state);

// Scheduler Interface
extern void             InitScheduler();                                    // Called once at the beginning
extern void             HandleNewTask(Time_t time, TaskId_t task_id);       // Called every time a new task arrives to the system
extern void             HandleTaskCompletion(Time_t time, TaskId_t task_id);// Called whenver a task finishes
extern void             MemoryWarning(Time_t time, MachineId_t machine_id); // Called to alert the scheduler of memory overcommitment
extern void             MigrationDone(Time_t time, VMId_t vm_id);           // Called to alert the scheduler that the VM has been migrated successfully
extern void             SchedulerCheck(Time_t time);                        // Called periodically. You may want to do some monitoring and adjustments
extern void             SimulationComplete(Time_t time);                    // Called at the end of the simulation
extern void             SLAWarning(Time_t time, TaskId_t task_id);          // Called to alert the schedule of an SLA violation
extern void             StateChangeComplete(Time_t time, MachineId_t machine_id);   // Called in response to an earlier request to change the state of a machine

// Statistics
extern double           GetSLAReport(SLAType_t sla);

// Simulator Interface
extern Time_t           Now();

// Task Interface
extern unsigned         GetNumTasks();
extern TaskInfo_t       GetTaskInfo(TaskId_t task_id);
extern unsigned         GetTaskMemory(TaskId_t task_id);
extern unsigned         GetTaskPriority(TaskId_t task_id);
extern bool             IsSLAViolation(TaskId_t);
extern bool             IsTaskCompleted(TaskId_t task_id);
extern bool             IsTaskGPUCapable(TaskId_t task_id);
extern CPUType_t        RequiredCPUType(TaskId_t task_id);
extern SLAType_t        RequiredSLA(TaskId_t task_id);
extern VMType_t         RequiredVMType(TaskId_t task_id);
extern void             SetTaskPriority(TaskId_t task_id, Priority_t priority);

// VM Interface
extern void             VM_Attach(VMId_t vm_id, MachineId_t machine_id);
extern void             VM_AddTask(VMId_t vm_id, TaskId_t task_id, Priority_t priority);
extern VMId_t           VM_Create(VMType_t vm_type, CPUType_t cpu);
extern VMInfo_t         VM_GetInfo(VMId_t vm_id);
extern void             VM_Migrate(VMId_t vm_id, MachineId_t machine_id);
extern void             VM_RemoveTask(VMId_t vm_id, TaskId_t task_id);
extern void             VM_Shutdown(VMId_t vm_id);

#endif /* Interfaces_h */
