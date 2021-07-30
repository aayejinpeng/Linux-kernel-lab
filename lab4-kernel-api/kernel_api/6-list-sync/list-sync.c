/*
 * Linux API lab
 *
 * list-sync.c - Synchronize access to a list
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched/signal.h>
#include <linux/spinlock.h>

MODULE_DESCRIPTION("Full list processing with synchronization");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct task_info {
	pid_t pid;
	unsigned long timestamp;
	atomic_t count;
	struct list_head list;
};

static struct list_head head;

/* TODO 1: you can use either a spinlock or rwlock, define it here */
spinlock_t list_lock;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	ti = kmalloc(sizeof(*ti), GFP_KERNEL);
	if (ti == NULL)
		return NULL;
	ti->pid = pid;
	ti->timestamp = jiffies;
	atomic_set(&ti->count, 0);

	return ti;
}

static struct task_info *task_info_find_pid(int pid)
{
	struct list_head *p;
	struct task_info *ti;

	list_for_each(p, &head) {
		ti = list_entry(p, struct task_info, list);
		if (ti->pid == pid) {
			return ti;
		}
	}

	return NULL;
}

static void task_info_add_to_list(int pid)
{
	struct task_info *ti;

	/* TODO 1: Protect list, is this read or write access? */
	spin_lock(&list_lock);
	ti = task_info_find_pid(pid);
	if (ti != NULL) {
		ti->timestamp = jiffies;
		atomic_inc(&ti->count);
		spin_unlock(&list_lock);
		/* TODO: Guess why this comment was added  here */
		return;
	}
	/* TODO 1: critical section ends here */
	spin_unlock(&list_lock);

	ti = task_info_alloc(pid);
	/* TODO 1: protect list access, is this read or write access? */
	spin_lock(&list_lock);
	list_add(&ti->list, &head);
	spin_unlock(&list_lock);
	/* TODO 1: critical section ends here */
}
EXPORT_SYMBOL(task_info_add_to_list);
void task_info_add_for_current(void)
{
	task_info_add_to_list(current->pid);
	task_info_add_to_list(current->parent->pid);
	task_info_add_to_list(next_task(current)->pid);
	task_info_add_to_list(next_task(next_task(current))->pid);
}
EXPORT_SYMBOL(task_info_add_for_current);
/* TODO 2: Export the kernel symbol */

void task_info_print_list(const char *msg)
{
	struct list_head *p;
	struct task_info *ti;

	pr_info("%s: [ ", msg);

	/* TODO 1: Protect list, is this read or write access? */
	spin_lock(&list_lock);
	list_for_each(p, &head) {
		ti = list_entry(p, struct task_info, list);
		pr_info("(%d, %lu) ", ti->pid, ti->timestamp);
	}
	/* TODO 1: Critical section ends here */
	spin_unlock(&list_lock);
	pr_info("]\n");
}
EXPORT_SYMBOL(task_info_print_list);
/* TODO 2: Export the kernel symbol */

void task_info_remove_expired(void)
{
	struct list_head *p, *q;
	struct task_info *ti;

	/* TODO 1: Protect list, is this read or write access? */
	spin_lock(&list_lock);	
	list_for_each_safe(p, q, &head) {
		ti = list_entry(p, struct task_info, list);
		if (jiffies - ti->timestamp > 10 * HZ && atomic_read(&ti->count) < 5) {
			list_del(p);
			kfree(ti);
		}
	}
	/* TODO 1: Critical section ends here */
	spin_unlock(&list_lock);
}
EXPORT_SYMBOL(task_info_remove_expired);
/* TODO 2: Export the kernel symbol */


static void task_info_purge_list(void)
{
	struct list_head *p, *q;
	struct task_info *ti;

	/* TODO 1: Protect list, is this read or write access? */
	spin_lock(&list_lock);
	list_for_each_safe(p, q, &head) {
		ti = list_entry(p, struct task_info, list);
		list_del(p);
		kfree(ti);
	}
	spin_unlock(&list_lock);
	/* TODO 1: Critical sections ends here */
}

static int list_sync_init(void)
{
	spin_lock_init(&list_lock);
	INIT_LIST_HEAD(&head);

	task_info_add_for_current();
	task_info_print_list("after first add");

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5 * HZ);

	return 0;
}

static void list_sync_exit(void)
{
	struct task_info *ti;

	ti = list_entry(head.prev, struct task_info, list);
	atomic_set(&ti->count, 10);

	task_info_remove_expired();
	task_info_print_list("after removing expired");
	task_info_purge_list();
}

module_init(list_sync_init);
module_exit(list_sync_exit);
