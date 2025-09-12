In this project, I added a system call and implemented two new schedulers (FCFS and CFS) in xv6.


Part A: getreadcount System Call

    I implemented a system call that tracks the total number of bytes read by the system since boot.

    kernel/syscall.h: Added #define SYS_getreadcount 22.
    kernel/file.c:
    Introduced a global counter uint total_read_bytes with a spinlock.
    Updated fileread() to increment the counter safely whenever a read happens.
    Implemented sys_getreadcount() to return the counter value safely.
    kernel/syscall.c: Linked the syscall number to sys_getreadcount.
    user/user.h & user/usys.pl: Declared the user-level getreadcount() function.
    user/readcount.c: Wrote a small test program that:
    Prints initial read count.
    Reads 100 bytes from a file.
    Calls getreadcount() again to verify the increase.
    Makefile: Added _readcount to UPROGS to include the test in the file system.


Part B-1: FCFS Scheduler

    FCFS is a non-preemptive scheduler that runs processes in the order they arrived.

    Makefile: Added support for SCHEDULER=FCFS by defining -DFCFS.
    kernel/proc.h: Added uint ctime to struct proc to store creation time.
    kernel/proc.c:
    In allocproc(): Set p->ctime = ticks for timestamping new processes.
    In scheduler(): Under #ifdef FCFS, looped through all processes to pick the RUNNABLE one with the smallest ctime


Part B-2: CFS Scheduler

    CFS gives fair CPU time by prioritizing processes that have run the least.

    Makefile: Support added for SCHEDULER=CFS.
    kernel/proc.h: Added fields int nice, int weight, uint64 vruntime, and int ticks_ran.
    kernel/proc.c:
    Set up a weights array to map nice values to weight.
    Initialized CFS-specific fields in allocproc().
    In scheduler(): Under #elif defined(CFS), always selected the RUNNABLE process with the smallest vruntime.
    kernel/trap.c:
    The timer interrupt was modified so that under CFS, a process runs for a dynamic time slice calculated as: time_slice = target_latency / runnable_process_count, with a minimum of 3 ticks.
    After the slice ends, vruntime is updated, and the process yields.



Performance comparison -

    Policy	    Avg Wait Time	Avg Run Time
    FCFS	        118	            17
    Round Robin	    150	            16
    CFS	            160	            18

FCFS had the lowest wait time (good for simple CPU-heavy tasks).
Round Robin balanced fairness and responsiveness.
CFS added overhead for fairness but works better in complex workloads.





Some log output - 

    xv6 kernel is booting

    hart 1 starting
    [Scheduler Tick]
    PID: 1 | vRuntime: 0
    hart 2 starting
    [Scheduler Tick]
    PID: 1 | vRuntime: 0
    --> Scheduling PID 1 (lowest vRuntime)

    init: starting sh
    [Scheduler Tick]
    PID: 2 | vRuntime: 0
    --> Scheduling PID 2 (lowest vRuntime)

    [Scheduler Tick]
    PID: 2 | vRuntime: 0
    --> Scheduling PID 2 (lowest vRuntime)

    [Scheduler Tick]
    PID: 3 | vRuntime: 0
    --> Scheduling PID 3 (lowest vRuntime)

    [Scheduler Tick]
    PID: 3 | vRuntime: 0
    --> Scheduling PID 3 (lowest vRuntime)