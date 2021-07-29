#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
/* TODO: add missing headers */

MODULE_DESCRIPTION("List current processes");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static int my_mem_info_init(void)
{
	struct vm_area_struct *p = current->active_mm->mmap;
	while(NULL != p->vm_next)
	{
		pr_info("active_mm = start %x; end %x\n",p->vm_start,p->vm_end);
		p = p->vm_next;
	}
	
	p = current->mm->mmap;
	while(NULL != p->vm_next)
	{
		pr_info("mm = start %x; end %x\n",p->vm_start,p->vm_end);
		p = p->vm_next;
	}
	
	/* TODO: print current process pid and its name */
	

	return 0;
}

static void my_mem_info_exit(void)
{
	/* TODO: print current process pid and name */
	struct task_struct *p;
	p = current;
	pr_info("exit: pid %d  name = %s\n", p->pid,p->comm);
}

module_init(my_mem_info_init);
module_exit(my_mem_info_exit);
