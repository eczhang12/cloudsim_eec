//
//  SimTypes.h
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 11/3/24.
//

#ifndef SimTypes_h
#define SimTypes_h

#include <iostream>
#include <vector>

using namespace std;

typedef uint64_t Time_t;          // Time is computed in microseconds
typedef uint64_t EventId_t;

typedef unsigned CPUId_t;
typedef unsigned MachineId_t;
typedef unsigned VMId_t;
typedef unsigned TaskId_t;

typedef enum {
    P0,         // CPU at normal frequency
    P1,         // CPU at 3/4 frequency, 0.8 voltage
    P2,         // CPU at 1/2 frequency, 0.7 voltage
    P3          // CPU at 1/4 frequency, 0.6 voltage
} CPUPerformance_t;
#define P_STATES 4

typedef enum {
    C0,         // CPU is at state C0, in this case the power consumption is defined by the P-states
    C1,         // CPU is at state C1 (halted but ready)
    C2,         // CPU is clocked gated off
    C4          // CPU is powered gated off, note: C3 is not supported
} CPUState_t;
#define C_STATES 4  // For C0, the power consumption is defined by the P-states

typedef enum {
    ARM,
    POWER,
    RISCV,
    X86
} CPUType_t;

typedef enum {
    S0,         // Machine is up. CPU's are at state C0 if running a task or C1
    S0i1,       // Machine is up. CPU's are all in C1 state. Instantenous response.
    S1,         // Machine is up. CPU's are in C2 state. Some delay in response time.
    S2,         // S1 + CPUs are in C4 state. Delay in response time.
    S3,         // S2 + DRAM in self-refresh. Serious delay in response time.
    S4,         // S3 + DRAM is powered down. Large delay in response time.
    S5          // Machine is powered down.
} MachineState_t;
#define S_STATES 7

typedef enum {
HIGH_PRIORITY,           // System has 3 levels of priority,
    MID_PRIORITY,
    LOW_PRIORITY
} Priority_t;
#define PRIORITY_LEVELS 3

typedef enum {
    SLA0,                   // SLA requires 95% of tasks to finish within expected time
    SLA1,                   // SLA requires 90% of tasks to finish within expected time
    SLA2,                   // SLA requires 80% of tasks to finish within expected time
    SLA3                    // Task to finish on a best effort basis
} SLAType_t;
#define NUM_SLAS 4

typedef enum {
    AI_TRAINING,            // Task is compute intensive, can benefit from GPU
    CRYPTO,                 // Task is compute intensive, short, but repepetive
    SCIENTIFIC,             // Task is compute intensive, very long, can benefit from GPU
    STREAMING,              // Long movie
    WEB_REQUEST             // Short task
} TaskClass_t;

typedef enum {
    LINUX,
    LINUX_RT,
    WIN,
    AIX
} VMType_t;
#define VM_MEMORY_OVERHEAD  8 

typedef struct {
    unsigned num_cpus;                      // Number of CPU's on the machine
    CPUType_t cpu;                          // CPU types deployed in the machine
    unsigned memory_size;                   // Size of memory
    unsigned memory_used;                   // The memory currently in use
    unsigned active_tasks;                  // Number of tasks that are assigned to this machine
    unsigned active_vms;                    // Number of virtual machines that are attached to this machine
    bool gpus;                              // True if the processors are equipped with a GPU, false otherwise
    uint64_t energy_consumed;               // How much energy has been consumed so far
    vector<unsigned> performance;           // The MIPS ratings for the CPUs at different p-state
    vector<unsigned> c_states;              // Power consumption under different C states
    vector<unsigned> p_states;              // Power consumption for cores at different P states. Valid only when C-state is C0.
    vector<unsigned> s_states;              // Machine power consumption under different S states
    MachineState_t s_state;                 // The current S state of the machine
    CPUPerformance_t p_state;               // The current P state of the CPUs (all CPUs are set to the same P state to simplify scheduling
    MachineId_t machine_id;                 // The identifier of the machine
} MachineInfo_t;

typedef struct {
    bool completed;

    uint64_t total_instructions;
    uint64_t remaining_instructions;
    Time_t arrival;
    Time_t completion;
    Time_t target_completion;
    bool gpu_capable;
    
    Priority_t priority;
    
    CPUType_t required_cpu;
    unsigned required_memory;
    SLAType_t required_sla;
    VMType_t required_vm;

    TaskId_t task_id;
} TaskInfo_t;

typedef struct {
    vector<TaskId_t> active_tasks;
    CPUType_t cpu;
    MachineId_t machine_id;
    VMId_t vm_id;
    VMType_t vm_type;
} VMInfo_t;

#endif /* SimTypes_h */
