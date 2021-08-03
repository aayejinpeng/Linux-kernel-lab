/*
 * Character device drivers lab
 *
 * All tasks
 */
#include <asm/atomic.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>


#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_INFO

#define MY_MAJOR		42
#define MY_MINOR		0
#define NUM_MINORS		1
#define MODULE_NAME		"so2_cdev"
#define MESSAGE			""
#define IOCTL_MESSAGE		"Hello ioctl"

#ifndef BUFSIZ
#define BUFSIZ		4096
#endif


struct so2_device_data {
	/* TODO 2: add cdev member */
	struct cdev cdev;
	/* TODO 4: add buffer with BUFSIZ elements */
	char buffer[BUFSIZ];
	/* TODO 7: extra members for home */
	spinlock_t access;
	char ioctk_buffer[BUFFER_SIZE];
	wait_queue_head_t wq;
	int wq_flag;
	/* TODO 3: add atomic_t access variable to keep track if file is opened */
	// atomic_t access;
};

struct so2_device_data devs[NUM_MINORS];

static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;

	/* TODO 2: print message when the device file is open. */
	
	pr_info("so2_cdev is open!\n");
	
	/* TODO 3: inode->i_cdev contains our cdev struct, use container_of to obtain a pointer to so2_device_data */
	data = container_of(inode->i_cdev, struct so2_device_data, cdev);
	file->private_data = (void *)data;

	/* TODO 3: return immediately if access is != 0, use atomic_cmpxchg */
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(1000);
	return 0;
}

static int
so2_cdev_release(struct inode *inode, struct file *file)
{
	/* TODO 2: print message when the device file is closed. */
	pr_info("so2_cdev is release!\n");
#ifndef EXTRA
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;

	/* TODO 3: reset access variable to 0, use atomic_set */
	spin_lock(&data->access);
	data->wq_flag = 0;
	spin_unlock(&data->access);
	wake_up(&data->wq);
	//atomic_set(&data->access,0);
#endif
	return 0;
}

static ssize_t
so2_cdev_read(struct file *file,
		char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	size_t to_read;

#ifdef EXTRA
	/* TODO 7: extra tasks for home */
#endif
	if((file->f_flags&O_NONBLOCK)&&data->wq_flag != 0) return -EWOULDBLOCK;
	while(1)
	{
		spin_lock(&data->access);
		if(data->wq_flag == 0)
		{
			data->wq_flag = 1;
			spin_unlock(&data->access);
			break;
		}
		spin_unlock(&data->access);
		wait_event(data->wq,!(data->wq_flag));
	}
	/* TODO 4: Copy data->buffer to user_buffer, use copy_to_user */
	ssize_t len = min(strlen(data->buffer) - *offset, size);
	//pr_info("buffer = %s",data->buffer);
	//pr_info("Offset: %lld, size: %u, len:%u", *offset,size,len);
	to_read = copy_to_user(user_buffer,data->buffer + *offset,len);
	//pr_info("toread: %u\n", to_read);
	*offset += len - to_read;
	//pr_info("user_buffer = %s",user_buffer);
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(1000);
	return len - to_read;
}

static ssize_t
so2_cdev_write(struct file *file,
		const char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	ssize_t len = min(BUFSIZ - *offset, size);
	ssize_t towrite;
	towrite = copy_from_user(data->buffer + *offset, user_buffer, len);
	*offset += towrite;
	/* TODO 5: copy user_buffer to data->buffer, use copy_from_user */
	/* TODO 7: extra tasks for home */

	return len - towrite;
}

static long
so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	int ret = 0;
	int remains;

	switch (cmd) {
	/* TODO 6: if cmd = MY_IOCTL_PRINT, display IOCTL_MESSAGE */
	case MY_IOCTL_PRINT:
	{
		pr_info(IOCTL_MESSAGE);
		break;
	}
	case MY_IOCTL_SET_BUFFER:
	{
		
		if(copy_from_user(data->ioctk_buffer,(void *)arg,sizeof(data->ioctk_buffer)))
			ret = -EFAULT;
		break;
	}
	case MY_IOCTL_UP:
	{
		wait_event(data->wq,data->wq_flag);
		pr_info("wait~im exit~\n");
		break;
	}
	case MY_IOCTL_DOWN:
	{
		pr_info("wake up!\n");
		data->wq_flag = 1;
		wake_up(&data->wq);
		break;
	}
	case MY_IOCTL_GET_BUFFER:
	{
		if(copy_to_user((void *)arg,data->ioctk_buffer,sizeof(data->ioctk_buffer)))
			ret = -EFAULT;
		break;
	}
	/* TODO 7: extra tasks, for home */
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations so2_fops = {
	.owner = THIS_MODULE,
/* TODO 2: add open and release functions */
	.open = so2_cdev_open,
	.release = so2_cdev_release,
	.read = so2_cdev_read,
	.write = so2_cdev_write,
	.unlocked_ioctl = so2_cdev_ioctl,
/* TODO 4: add read function */
/* TODO 5: add write function */
/* TODO 6: add ioctl function */
};

static int so2_cdev_init(void)
{
	int err;
	int i;

	/* TODO 1: register char device region for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
	err = register_chrdev_region(MKDEV(MY_MAJOR,0),NUM_MINORS,MODULE_NAME);
	if (err) pr_info("%d\n",err);
	for (i = 0; i < NUM_MINORS; i++) {
#ifdef EXTRA
		/* TODO 7: extra tasks, for home */
#else
		/*TODO 4: initialize buffer with MESSAGE string */
		strcpy(devs[i].buffer,MESSAGE);
#endif
		/* TODO 7: extra tasks for home */
		/* TODO 3: set access variable to 0, use atomic_set */
		/* TODO 2: init and add cdev to kernel core */
		cdev_init(&devs[i].cdev,&so2_fops);
		cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
		init_waitqueue_head(&devs[i].wq);
		devs[i].wq_flag = 0;
		spin_lock_init(&devs[i].access);
		//atomic_set(&devs[i].access,0);
	}
	pr_info("aha!the handsome man get the so_cdev init~!\n");
	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

	for (i = 0; i < NUM_MINORS; i++) {
		/* TODO 2: delete cdev from kernel core */
		cdev_del(&devs[i].cdev);
	}

	/* TODO 1: unregister char device region, for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
	pr_info("aha!the handsome man get the so_cdev exit~!\n");
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
