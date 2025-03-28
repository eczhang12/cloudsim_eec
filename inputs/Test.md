machine class:
{
        Number of machines: 1
        CPU type: X86
        Number of cores: 16
        Memory: 8192
        S-States: [40, 20, 16, 12, 10, 4, 0]
        P-States: [4, 2, 2, 1]
        C-States: [4, 1, 1, 0]
        MIPS: [1500, 1200, 1000, 600]
        GPUs: no
}

task class:
{
        Start time: 60000
        End time : 600000000
        Inter arrival: 100000
        Expected runtime: 1000000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA1
        CPU type: X86
        Task type: HPC
        Seed: 520230
}
