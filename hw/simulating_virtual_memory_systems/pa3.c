/**********************************************************************
 * Copyright (c) 2019
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <memory.h>


#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * The current process
 */
extern struct process *current;

/**
 * alloc_page()
 *
 * DESCRIPTION
 *   Allocate a page from the system. This function is implemented in vm.c
 *   and use to get a page frame from the system.
 *
 * RETURN
 *   PFN of the newly allocated page frame.
 */
extern unsigned int alloc_page(void);



/**
 * TODO translate()
 *
 * DESCRIPTION
 *   Translate @vpn of the @current to @pfn. To this end, walk through the
 *   page table of @current and find the corresponding PTE of @vpn.
 *   If such an entry exists and OK to access the pfn in the PTE, fill @pfn
 *   with the pfn of the PTE and return true.
 *   Otherwise, return false.
 *   Note that you should not modify any part of the page table in this function.
 *
 * RETURN
 *   @true on successful translation
 *   @false on unable to translate. This includes the case when @rw is for write
 *   and the @writable of the pte is false.
 */

// ./vm testcases/simple
// gdb ./vm
// r 1
bool translate(enum memory_access_type rw, unsigned int vpn, unsigned int *pfn)
{
	/*** DO NOT MODIFY THE PAGE TABLE IN THIS FUNCTION ***/
	struct pte_directory *pd =NULL;
	struct pte *pte =NULL;
	int oidx = vpn / 16;
	int iidx = vpn % 16;

	if (!current)
	{
		current = malloc(sizeof(struct process));
	}

	if (current->pagetable.outer_ptes[oidx])
	{
		pte = &(current->pagetable.outer_ptes[oidx])->ptes[iidx];
		if ( pte->valid )
		{
			if ((!pte->writable && rw == WRITE))
				return false;
			*pfn = pte->pfn;
			return true;
		}
	}
	// printf("pd: %p\n", pd);
	// printf("pte: %p\n", pte);
	// printf("translate fails");
	return false;
}


/**
 * TODO handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the translate() for @vpn fails. This implies;
 *   1. Corresponding pte_directory is not exist
 *   2. pte is not valid
 *   3. pte is not writable but @rw is for write
 *   You can assume that all pages are writable; this means, when a page fault
 *   happens with valid PTE without writable permission, it was set for the
 *   copy-on-write.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(enum memory_access_type rw, unsigned int vpn)
{
	// printf("%d\n",rw);
	int oidx = vpn / 16;
	int iidx = vpn % 16;
	struct pte_directory *pd  = current->pagetable.outer_ptes[oidx];
	struct pte *pte = &pd->ptes[iidx];

	if (!pd)
	{
		// printf("There is not pd111111\n");
		pd = malloc(sizeof(struct pte_directory));
		current->pagetable.outer_ptes[oidx] =pd;
		pte = &pd->ptes[iidx];
		pte->pfn = alloc_page();
		// printf("1. %ud\n", pte->pfn);
		pte->valid = true;
		pte->writable = true;
		// free(pd);
		// printf("There is not pd2222\n");
		
	}
	else if(!pte->valid && rw == READ)
	{
		// printf("pte is not valid111111\n");
		
		pd = current->pagetable.outer_ptes[oidx];
		pte = &pd->ptes[iidx];
		pte->pfn = alloc_page();
		// printf("2. %ud\n", pte->pfn);
		pte->valid = true;
		pte->writable = true;
		// printf("pte is not valid22222\n");
	}
	else if (!pte->writable && rw == WRITE)
	{
		// printf("pte is not writable111111\n");
		
		pd = current->pagetable.outer_ptes[oidx];
		pte = &pd->ptes[iidx];
		pte->pfn = alloc_page();

		// printf("3. %ud\n", pte->pfn);
		pte->valid = true;
		pte->writable = true;
		// printf("pte is not writable22222\n");
	}
	// free(pd);
	return true;
}


/**
 * TODO switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put to the **TAIL** of the
 *   @processes list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes.
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. Also, should update the writable bit properly to implement
 *   the copy-on-write feature.
 */
void switch_process(unsigned int pid)
{

	struct process *child = NULL;
	struct process *tmp = NULL;
	struct pte_directory *pd = NULL;
	struct pte_directory *child_pd = NULL;
	struct pte *child_pte = NULL;
	struct pte *pte = NULL;

	list_add_tail(&current->list, &processes);

	if (!list_empty(&processes))
	{
		list_for_each_entry(tmp, &processes, list)
		{
			printf("%d\n", tmp->pid);
			if(tmp->pid == pid)
			{
				current = tmp;
				printf("%d\n",current->pid);
				return;
			}
			else
			{
				child = malloc(sizeof(struct process));
				// child_pd = malloc(sizeof(struct pte_directory));

				memcpy(child, current, sizeof(struct process));
				child->pid = pid;
				
				for (int i = 0; i < NR_PTES_PER_PAGE; i++)
				{
					pd = child->pagetable.outer_ptes[i];
					if(!pd) continue;
					for (int j = 0; j < NR_PTES_PER_PAGE; j++)
					{
						pte = &pd->ptes[j];
						if(!pte) continue;

						// printf("writable? %d %d\n", pte->valid, pte->writable);
						pte->writable = false;
					}
				}
				current = child;
				 list_add_tail(&current->list, &processes);
				// printf("switch");
			}
		}
		
	}
	
}

