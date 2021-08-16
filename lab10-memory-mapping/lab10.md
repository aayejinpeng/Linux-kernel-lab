## lab10-Memory mapping

### 练习部分：

##### 1. Mapping contiguous physical memory to userspace

```c
remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)kmalloc_area) >> PAGE_SHIFT, length, vma->vm_page_prot);//将虚拟地址转换为物理地址，然后获取它的页匡号，再映射
........................................................................................
kmalloc_ptr = kmalloc((NPAGES+2)*PAGE_SIZE,GFP_KERNEL);	//申请连续的内存
kmalloc_area = (char *) PAGE_ALIGN(((unsigned long)kmalloc_ptr));;	//找到页对齐的初始地址
```

##### 2. Mapping non-contiguous physical memory to userspace

```c
int i = 0;
for(i = 0; i < length; i+=PAGE_SIZE)
{
    vmalloc_area_ptr = vmalloc_area + i;
    ret = remap_pfn_range(vma, vma->vm_start + i, vmalloc_to_pfn((void *)vmalloc_area_ptr), length>i+PAGE_SIZE?PAGE_SIZE:length-i, vma->vm_page_prot);
}
//根据每个单独的虚拟页，获取对应的页框号，然后将其映射
........................................................................................
vmalloc_area = vmalloc(NPAGES*PAGE_SIZE);	//申请非连续的内存
for(i = 0; i<NPAGES; i++)
{
    //对非对齐的页每页的标志位进行修改
    SetPageReserved(vmalloc_to_page((unsigned long)vmalloc_area + i*PAGE_SIZE));
    //修改每页的前4个数据
    *(vmalloc_area + i*PAGE_SIZE + 0) = 0xaa;
    *(vmalloc_area + i*PAGE_SIZE + 1) = 0xbb;
    *(vmalloc_area + i*PAGE_SIZE + 2) = 0xcc;
    *(vmalloc_area + i*PAGE_SIZE + 3) = 0xdd;
}
```

##### 3. Read / write operations in mapped memory

##### 4. Display memory mapped in procfs

```c
//mm描述的是当前进程的内存映射情况，vma对应了每个映射区间，遍历即可，还有红黑树可以用于确定特定的节点
/* TODO 3: Get current process' mm_struct */
	mm = get_task_mm(current);
	/* TODO 3: Iterate through all memory mappings */
	vma_iterator = mm->mmap;
	while(vma_iterator)
	{
		pr_info("%lx %lx",vma_iterator->vm_start,vma_iterator->vm_end);
		total -= vma_iterator->vm_start - vma_iterator->vm_end;
		vma_iterator = vma_iterator->vm_next;
	}
	/* TODO 3: Release mm_struct */
	mmput(mm);
	/* TODO 3: write the total count to file  */
	seq_printf(seq, "%lu %s\n", total, current->comm);

```

