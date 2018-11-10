# # Spring 2018 :: CSE 422S Lab 1 - Preemption Monitoring Framework
This lab shows some ways to perform monitoring tasks, using kernel modules, scheduler notifications, and file system operations to communicate information betweeen user-space and kernel-space.

# # Objectives
1. Use a kernel module to dynamically add code to a running kernel.
2. Implement callbacks for scheduler notifications, in order to determine how and when a user program is preempted by another task.
3. Create a kernel-level monitoring framework that leverages scheduler notifications and linked lists to maintain a detailed log of all of the preemptions experienced by a process.
4. Implement device file I/O callbacks in order to communicate kernel-level information to user-space.
5. Evaluate the operation of your monitoring framework through experimentation with user-level programs and different kernel configuration options.


# # Kernel Configuration
### Enabled options:
- MMU
- OF
- ARM_VIRT_EXT
- ARM_ARCH_TIMER

### disabled options:
- KVM
- VIRTUALIZATION
- ARM_LPAE

#### Enabling previously disabled options:
- VIRTUALIZATION does not depend on any other options.
- ARM_LPAE depends on CPU_32v7, which is already enabled.
- CONFIG_VIRTUALIZATION=y, CONFIG_ARM_LPAE=y, CONFIG_KVM=y in the .config file

# # Module Bootstrapping
### Added code:
- tracker = kmalloc(sizeof(struct preemption_tracker), GFP_KERNEL);
- preempt_notifier_init(&tracker->notifier, &notifier_ops);
- preempt_notifier_unregister(&tracker->notifier);
- preempt_notifier_register(&tracker->notifier);
- preempt_notifier_unregister(&tracker->notifier);

### Deleted code:
- All the error message and the return error numbers

### dmesg output:
- [ 1361.314113] process 1322 (monitor) closed /dev/sched_monitor

# # Module Design
#### Our preemption entry holds the following variables:
- unsigned long long *on_core_time* - time it runs before getting preempted
- unsigned long long *wait_time* - time it waits before running again
- unsigned long long *schedin_time* - time it gets scheduled in
- unsigned long long *schedout_time* - time it gets scheduled out
- unsigned int *on_which_core* - which cpu is it running on
- char* *name* - name of the task that preempts the process

When we open the schedule monitor, we register the tracker as the head of the list of the entries. Then as the process gets preempted, in the *monitor_sched_out* function, we create a new entry for the process, save the current time in *schedout_time*, record the name of the task that preempted the process, and instantiate the *on_core_time* and *wait_time* as following:

- *on_core_time* = current time - previous preempted entry's scheduled in time
- *wait_time* = previous preempted entry's scheduled in time - previous preempted entry's scheduled out time

#### * Visual explanation:
        [out			in]    [out			in]
    |******|			|**************|			|*************|
    |----------------------------------------------------------------------|
Treating a complete bracket as one node, we see that the wait time of the current node is previous entry's scheduled in time minus the previous entry's scheduled out time. The run time of the current node is current preempted time minus the previous entry's scheduled in time. We append the node to the list. Then in the *monitor_sched_in* function as the process gets scheduled back in, we record the cpu number that it runs on in and record the scheduled in time.

# # dmesg output
> [ 1786.502343] sched_out for process monitor
[ 1786.502351] sched_out:: core:9835040 bwn:163488 cpu:0 comm:in:imklog
[ 1786.502541] sched_in for process monitor
[ 1786.502549] sched_in:: core:9835040 btw:163488 pmt:1520642634110050401 cpu:3 comm:in:imklog
[ 1786.511112] length::86
[ 1786.511118] Deleting node while writing:: core:9835040 btw:163488 cpu:3 comm:in:imklog
[ 1786.511126] copied::0
[ 1786.511288] length::86
[ 1786.511296] Deleting node while writing:: core:9767019 btw:231457 cpu:3 comm:in:imklog
[ 1786.511303] copied::0
[ 1786.511360] Deleting node : 9958267 44115 3 rcu_preempt
[ 1786.511366] Deleting node : 9941132 61354 3 in:imklog
[ 1786.511353] Process 1379 (monitor) closed /dev/sched_monitor

# # Monitor program user output
> pi@anniechaehonglee:~/Desktop/userspace $ sudo ./monitor 1
on_core_time:9793342
wait_time:213177
which_cpu:3
name:rs:main Q:Reg
Monitor ran to completion!

# # Results
When run with time command, without any other program running, the dense_mm program exists in ~30 seconds

### Example output of the matrix program:

##### * With just the matrix program:
> Program summary:
avg run time: 132128510ns
avg wait time: 172198ns
num of migrations: 5
total preemptions: 213

==> This adds up to 28.18s

##### * With fibonacci running on 3 cores:
> Program summary:
avg run time: 27301267ns
avg wait time: 223898ns
num of migrations: 1
total preemptions: 1071

##### * With fibonacci running on 4 cores:
> Program summary:
avg run time: 40132231ns
avg wait time: 239317ns
num of migrations: 29
total preemptions: 1131


#### * With dense_mm priority set to -19:
> Program summary:
avg run time: 61237747ns
avg wait time: 256241ns
num of migrations: 10
total preemptions: 640


When *dense_mm* runs with other programs running, the process gets preempted more often, migration happens more, average runtime becomes smaller, and average wait time becomes bigger. When all four cores are reserved, the number of preemptions and the number of migrations increase. However, when priority is set so that the matrix program has higher priority, the program migrates less, gets preempted less (still more than when there are no programs running), and gets more average runtime. When there are programs running, the average wait time stays within the similar range.

*rcu_preempt* happens around the first and end of the list. When the matrix size is below 200, the task doesn't appear. When it is above 300, the task appears. It seems like for a process to be preempted by this task the wait time is around 200000ns. We think that this task preempts a process once it starts waiting for too long.

### Example output of the matrix program ran on 1000HZ setting linux:
#### * With just the matrix program:
> Program summary:
avg run time: 75822688ns
avg wait time: 86479ns
num of migrations: 4
total preemptions: 378

#### * With fibonacci running on 3 cores:
> Program summary:
avg run time: 6607306ns
avg wait time: 157224ns
num of migrations: 9
total preemptions: 4286

#### * With fibonacci running on 4 cores:
> Program summary:
avg run time: 18925892ns
avg wait time: 181875ns
num of migrations: 7
total preemptions: 567

#### * With dense_mm priority set to -19:
> Program summary:
avg run time: 174382447ns
avg wait time: 147339ns
num of migrations: 14
total preemptions: 506

When the matrix program is run on 1000HZ-configured linux machine, the average runtime and the wait time is significantly smaller than that of the program run on 100HZ-configured machine. This is because on higher hertz kernel, a process gets preempted sooner, so the run time and wait time decreases.

# # Contributors

* [Hakkyung Lee][HL] (hakkyung@wustl.edu)
* [Annie Chaehong Lee][AL] (annie.lee@wustl.edu)

[HL]: <https://github.com/hklee93>
[AL]: <https://github.com/anniechaehonglee>