machine class:
{
# comment
        Number of machines: 16
        CPU type: X86
        Number of cores: 8
        Memory: 16384
        S-States: [120, 100, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1000, 800, 600, 400]
        GPUs: yes
}


# machine class:
# {
#         Number of machines: 24
#         Number of cores: 16
#         CPU type: ARM
#         Memory: 16384
#         S-States: [120, 100, 100, 80, 40, 10, 0]
#         P-States: [12, 8, 6, 4]
#         C-States: [12, 3, 1, 0]
#         MIPS: [1000, 800, 600, 400]
#         GPUs: yes
# }


task class:
{
        Start time: 60000
        End time : 800000
        Inter arrival: 6000
        Expected runtime: 300000
        Memory: 100
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

task class:
{
        Start time: 60000
        End time : 800000
        Inter arrival: 6000
        Expected runtime: 300000
        Memory: 50
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}



task class:
{
        Start time: 60000
        End time : 800000
        Inter arrival: 6000
        Expected runtime: 300000
        Memory: 10
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}



