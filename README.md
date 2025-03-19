This is the repository for the Cloud Simulator project for CS 378. To run this project, you can compile the Scheduler with make scheduler and run make simulator to create your simulator executable. Run ./simulator Input.md to see your results.

For questions, please reach out to any of the course staff on via email (anish.palakurthi@utexas.edu, tarun.mohan@utexas.edu, mootaz@austin.utexas.edu) or Ed Discussion.


make simulator && make scheduler && ./simulator Input.md


round robin results: 
az9896@bloodstone:~/Downloads/cs378ee/cloudsim_eec$ make simulator && make scheduler && ./simulator Input.md
make: 'simulator' is up to date.
make: 'scheduler' is up to date.
SLA violation report
SLA0 : 40.625%
SLA1 : 0%
SLA2 : 0%
Total Energy 0.044995KW-Hour
Simulation run finished in 32.7 seconds


Greedy solution NOT FINAL
SLA violation report
SLA0 : 6.20155%
SLA1 : 0%
SLA2 : 0%
Total Energy 0.0196017KW-Hour
Simulation run finished in 30.62 seconds
Caught an exception!
VM::Shutdown(): Shutting down a VM while tasks are still running--likely a bug