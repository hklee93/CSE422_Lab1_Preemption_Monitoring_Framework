#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/string.h>
#include <asm/uaccess.h>


/* DO NOT MODIFY THIS CHECK. If you see the message in the error line below
 * when compliling your module, that means your kernel has not been configured
 * correctly. Complete exercises 1-3 of the lab before continuing
 */
#ifdef CONFIG_PREEMPT_NOTIFIERS
#include <linux/preempt.h>
#else
#error "Your kernel must be built with CONFIG_PREEMPT_NOTIFIERS enabled"
#endif

#include <sched_monitor.h>

#define MAX_PREEMPTS 32768

struct preemption_tracker
{
    /* notifier to register us with the kernel's callback mechanisms */
	struct preempt_notifier notifier;
	bool enabled;
	struct list_head head;
	spinlock_t my_lock;
    /* TODO: anything else you need */
};


/* Information tracked on each preemption. */
struct preemption_entry
{
    /* TODO: populate this */
	unsigned long long on_core_time;
	unsigned long long wait_time;
	unsigned long long schedin_time;
	unsigned long long schedout_time;
	unsigned int on_which_core;
	char* name;
	struct list_head list_node;
};


/* Get the current time, in nanoseconds. Should be consistent across cores.
 * You are encouraged to look at the options in:
 *      include/linux/timekeeping.h
 */
static inline unsigned long long
get_current_time(void)
{
    /* TODO: implement */
	struct timespec ts;
	getnstimeofday(&ts);
	return (unsigned long long) timespec_to_ns(&ts);
}

/*
 * Utilities to save/retrieve a tracking structure for a process
 * based on the kernel's file pointer.
 *
 * DO NOT MODIFY THE FOLLOWING 2 FUNCTIONS
 */
static inline void
save_tracker_of_process(struct file               * file,
                        struct preemption_tracker * tracker)
{
    file->private_data = tracker;
}

static inline struct preemption_tracker *
retrieve_tracker_of_process(struct file * file)
{
    return (struct preemption_tracker *)file->private_data;
}

/*
 * Utility to retrieve a tracking structure based on the
 * preemption_notifier structure.
 *
 * DO NOT MODIFY THIS FUNCTION
 */
static inline struct preemption_tracker *
retrieve_tracker_of_notifier(struct preempt_notifier * notifier)
{
    return container_of(notifier, struct preemption_tracker, notifier);
}

/*
 * Callbacks for preemption notifications.
 *
 * monitor_sched_in and monitor_sched_out are called when the process that
 * registered a preemption notifier is either scheduled onto or off of a
 * processor.
 *
 * You will use these functions to create a linked_list of 'preemption_entry's
 * With this list, you must be able to represent information such as:
 *      (i)   the amount of time a process executed before being preempted
 *      (ii)  the amount of time a process was scheduled out before being
 *            scheduled back on
 *      (iii) the cpus the process executes on each time it is scheduled
 *      (iv)  the number of migrations experienced by the process
 *      (v)   the names of the processes that preempt this process
 *
 * The data structure 'preemption_tracker' will be necessary to track some of
 * this information.  The variable 'pn' provides unique state that persists
 * across invocations of monitor_sched_in and monitor_sched_out. Add new fields
 * to it as needed to provide the information above.
 */

static void
monitor_sched_in(struct preempt_notifier * pn,
                 int                       cpu)
{

    struct preemption_tracker * tracker = retrieve_tracker_of_notifier(pn);
	struct preemption_entry * cur_entry;

    printk(KERN_INFO "sched_in for process %s\n", current->comm);
    /* TODO: record information as needed */
	cur_entry = list_entry((&tracker->head)->next, struct preemption_entry, list_node);
	cur_entry->schedin_time = get_current_time();
	cur_entry->on_which_core = cpu;

	printk("sched_in:: core:%llu btw:%llu pmt:%llu cpu:%d comm:%s\n", cur_entry->on_core_time, cur_entry->wait_time, cur_entry->schedin_time, cur_entry->on_which_core, cur_entry->name);

 }

static void
monitor_sched_out(struct preempt_notifier * pn,
                  struct task_struct      * next)
{
    struct preemption_entry *my_entry;
    struct preemption_tracker * tracker = retrieve_tracker_of_notifier(pn);
	struct preemption_entry *before;
    printk(KERN_INFO "sched_out for process %s\n", current->comm);

    /* TODO: record information as needed */

	before = list_entry((&tracker->head)->next, struct preemption_entry, list_node);

	my_entry = kmalloc(sizeof(struct preemption_entry), GFP_ATOMIC);
	my_entry->schedout_time = get_current_time();
	my_entry->on_core_time = get_current_time() - before->schedin_time;
	my_entry->wait_time = before->schedin_time - before->schedout_time;
	my_entry->name = next->comm;

	list_add(&my_entry->list_node, &tracker->head);

	printk("sched_out:: core:%llu bwn:%llu cpu:%d comm:%s\n", my_entry->on_core_time, my_entry->wait_time, my_entry->on_which_core, my_entry->name);


}

static struct preempt_ops
notifier_ops =
{
    .sched_in  = monitor_sched_in,
    .sched_out = monitor_sched_out
};

/*** END preemption notifier ***/


/*
 * Device I/O callbacks for user<->kernel communication
 */

/*
 * This function is invoked when a user opens the file /dev/sched_monitor.
 * You must update it to allocate the 'tracker' variable and initialize
 * any fields that you add to the struct
 */
static int
sched_monitor_open(struct inode * inode,
                   struct file  * file)
{
    struct preemption_tracker * tracker;

    /* TODO: allocate a preemption_tracker for this process, other initialization
     * as needed. The rest of this function will trigger a kernel oops until
     * you properly allocate the variable 'tracker'
     */

	tracker = kmalloc(sizeof(struct preemption_tracker), GFP_KERNEL);
	preempt_notifier_init(&tracker->notifier, &notifier_ops);
	tracker->enabled = false;
	INIT_LIST_HEAD(&tracker->head);
    /* setup tracker */
    /* initialize preempt notifier object */

    /* Save tracker so that we can access it on other file operations from this process */
    save_tracker_of_process(file, tracker);

	spin_lock_init(&tracker->my_lock);

    return 0;
}

/* This function is invoked when a user closes /dev/sched_monitor.
 * You must update is to free the 'tracker' variable allocated in the
 * sched_monitor_open callback, as well as free any other dynamically
 * allocated data structures you may have allocated (such as linked lists)
 */
static int
sched_monitor_flush(struct file * file,
                    fl_owner_t    owner)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_process(file);
	struct preemption_entry *watch, *next;



    /* Unregister notifier */
    if (tracker->enabled) {
        tracker->enabled = false;
	preempt_notifier_unregister(&tracker->notifier);
    }

	list_for_each_entry_safe(watch, next, &tracker->head, list_node){
		printk("Deleting node :: core:%llu btw:%llu cpu:%d comm:%s\n", watch->on_core_time, watch->wait_time, watch->on_which_core, watch->name);
		list_del(&watch->list_node);
	}
	printk(KERN_DEBUG "Process %d (%s) closed " DEV_NAME "\n",
		current->pid, current->comm);

	kfree(tracker);
	return 0;
}

/*
 * Enable/disable preemption tracking for the process that opened this file.
 * Do so by registering/unregistering preemption notifiers.
 */
static long
sched_monitor_ioctl(struct file * file,
                    unsigned int  cmd,
                    unsigned long arg)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_process(file);

    switch (cmd) {
        case ENABLE_TRACKING:
            if (tracker->enabled) {
                printk(KERN_ERR "Tracking already enabled for process %d (%s)\n",
                    current->pid, current->comm);
                return 0;
            }

            /* TODO: register notifier, set enabled to true, and remove the error return */

		tracker->enabled = true;
		preempt_notifier_register(&tracker->notifier);
	/*
            printk(KERN_DEBUG "Process %d (%s) trying to enable preemption tracking via " DEV_NAME ". Not implemented\n",
                current->pid, current->comm);

            return -ENOIOCTLCMD;
	*/
            break;


        case DISABLE_TRACKING:
            if (!tracker->enabled) {
                printk(KERN_ERR "Tracking not enabled for process %d (%s)\n",
                    current->pid, current->comm);
                return 0;
            }

            /* TODO: unregister notifier, set enabled to false, and remove the error return */

		tracker->enabled = false;
		preempt_notifier_unregister(&tracker->notifier);
	/*
            printk(KERN_DEBUG "Process %d (%s) trying to disable preemption tracking via " DEV_NAME ". Not implemented\n",
                current->pid, current->comm);

            return -ENOIOCTLCMD;
	*/
            break;

        default:
            printk(KERN_ERR "No such ioctl (%d) for " DEV_NAME "\n", cmd);
            return -ENOIOCTLCMD;
    }

    return 0;
}

/* User read /dev/sched_monitor
 *
 * In this function, you will copy an entry from the list of preemptions
 * experienced by the process to user-space.
 */
static ssize_t
sched_monitor_read(struct file * file,
                   char __user * buffer,
                   size_t        length,
                   loff_t      * offset)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_process(file);
    unsigned long flags;
	struct preemption_entry *watch, *next;
	unsigned long copied = 0;
	size_t counter;

	if(length % sizeof(preemption_info_t) != 0){ //buffer size check needed

		printk("buffer size not valid\n");
		return -ENODEV;
	}
	spin_lock_irqsave(&tracker->my_lock, flags);

	if((&tracker->head)->next != (&tracker->head)->prev){

		counter = 0;

		list_for_each_entry_safe(watch, next, &tracker->head, list_node){

			printk("Deleting node :: core:%llu btw:%llu cpu:%d comm:%s\n", watch->on_core_time, watch->wait_time, watch->on_which_core, watch->name);

			copied += copy_to_user(buffer + counter, watch, sizeof(preemption_info_t));
			strncpy(buffer + sizeof(preemption_info_t) - (sizeof(char) * 50), watch->name, 50);
			list_del(&watch->list_node);
			kfree(watch);
			counter += sizeof(preemption_info_t);
			if(counter == length){
				break;
			}
		}
	}
	else{
		spin_unlock_irqrestore(&tracker->my_lock, flags);
		printk("no more copies::%lu\n", copied);
		return -1;
	}

	spin_unlock_irqrestore(&tracker->my_lock, flags);
	return copied;

    /* TODO:
     * (1) make sure length is valid. It must be an even multiuple of the size of preemption_info_t.
     *     i.e., if the value is the size of the structure times 2, the user is requesting the first
     *     2 entries in the list
     * (2) lock your linked list
     * (3) retrieve the head of the linked list, if it is not empty
     * (4) copy the associated contents to user space
     * (5) remove the head of the linked list and free its dynamically allocated
     *     storage.
     * (6) repeat for each entry
     * (7) unlock the list
     */




    printk(KERN_DEBUG "Process %d (%s) read " DEV_NAME "\n",
        current->pid, current->comm);

    /* Use these functions to lock/unlock our list to prevent concurrent writes
     * by the preempt notifier callouts
     *
     *  spin_lock_irqsave(<pointer to spinlock_t variable>, flags);
     *  spin_unlock_irqrestore(<pointer to spinlock_t variable>, flags);
     *
     *  Before the spinlock can be used, it will have to be initialized via:
     *  spin_lock_init(<pointer to spinlock_t variable>);
     */

}

static struct file_operations
dev_ops =
{
    .owner = THIS_MODULE,
    .open = sched_monitor_open,
    .flush = sched_monitor_flush,
    .unlocked_ioctl = sched_monitor_ioctl,
    .compat_ioctl = sched_monitor_ioctl,
    .read = sched_monitor_read,
};

static struct miscdevice
dev_handle =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = SCHED_MONITOR_MODULE_NAME,
    .fops = &dev_ops,
};
/*** END device I/O **/



/*** Kernel module initialization and teardown ***/
static int
sched_monitor_init(void)
{
    int status;

    /* Create a character device to communicate with user-space via file I/O operations */
    status = misc_register(&dev_handle);
    if (status != 0) {
        printk(KERN_ERR "Failed to register misc. device for module\n");
        return status;
    }

    /* Enable preempt notifiers globally */
    preempt_notifier_inc();

    printk(KERN_INFO "Loaded sched_monitor module. HZ=%d\n", HZ);

    return 0;
}

static void
sched_monitor_exit(void)
{
    /* Disable preempt notifier globally */
    preempt_notifier_dec();

    /* Deregister our device file */
    misc_deregister(&dev_handle);

    printk(KERN_INFO "Unloaded sched_monitor module\n");
}

module_init(sched_monitor_init);
module_exit(sched_monitor_exit);
/*** End kernel module initialization and teardown ***/


/* Misc module info */
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Hakkyung Lee, Chaehong Lee");
MODULE_DESCRIPTION ("CSE 422S Lab 1");
