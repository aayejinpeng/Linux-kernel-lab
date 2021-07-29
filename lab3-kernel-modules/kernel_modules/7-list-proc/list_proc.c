#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
/* TODO: add missing headers */

MODULE_DESCRIPTION("List current processes");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static int my_proc_init(void)
{
	struct task_struct *p;
	p = current;
	
	/* TODO: print current process pid and its name */
	pr_info("init: pid %d  name = %s\n", p->pid,p->comm);

	/* TODO: print the pid and name of all processes */
	p = (struct task_struct *)((void *)(p->tasks.next) - (void *)(&((struct task_struct *)(0))->tasks));
	while(p != current)
	{
		pr_info("all: pid %d  name = %s\n", p->pid,p->comm);
		p = (struct task_struct *)((void *)(p->tasks.next) - (void *)(&((struct task_struct *)(0))->tasks));
	}
	return 0;
}

static void my_proc_exit(void)
{
	/* TODO: print current process pid and name */
	struct task_struct *p;
	p = current;
	pr_info("exit: pid %d  name = %s\n", p->pid,p->comm);
}

module_init(my_proc_init);
module_exit(my_proc_exit);
