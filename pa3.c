/**********************************************************************
 * Copyright (c) 2020-2021
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

#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr;


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with RW_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with RW_READ only should not be accessed with
 *   RW_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
	unsigned int pos = vpn % NR_PTES_PER_PAGE;
	unsigned int pfn = 0;
	unsigned int cnt = 1;
	*ptbr = current->pagetable;
	
	// marking
	unsigned int store[NR_PAGEFRAMES] = {0, };
	for (int i = 0; i < NR_PAGEFRAMES; i++) {
	    if (mapcounts[i] > 0) {
		store[i] = 1;
	    }
	}

	// check if all page frames are allocated
	for (int i = 0; i < NR_PAGEFRAMES; i++) {
	    if (store[i] == 1)	cnt++; 
	}
	if (cnt == NR_PAGEFRAMES) return -1;

	// search for the minimum ptr value
	for (int i = 0; i < NR_PAGEFRAMES; i++) {
	    if (store[i] == 0) {
		pfn = i;
		break;
	    }
	}
    
	for (int i = 0; i < NR_PTES_PER_PAGE; i++)
	{
	    if (vpn >= i * NR_PTES_PER_PAGE && vpn < (i+1) * NR_PTES_PER_PAGE) {
		if (!ptbr->outer_ptes[i]) {		    
		    ptbr->outer_ptes[i] = (struct pte_directory*)malloc(sizeof(struct pte_directory));
		}
		
		ptbr->outer_ptes[i]->ptes[pos].valid = true;
		if (rw == RW_READ) {
		    ptbr->outer_ptes[i]->ptes[pos].writable = false;
		} else if (rw == (RW_WRITE | RW_READ)) {
		    ptbr->outer_ptes[i]->ptes[pos].writable = true;
		}
		ptbr->outer_ptes[i]->ptes[pos].pfn = pfn;
		mapcounts[pfn] = mapcounts[pfn] + 1;	
	    }
	}

	return pfn;
}


/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, writable, pfn) is set @false or 0.
 *   Also, consider carefully for the case when a page is shared by two processes,
 *   and one process is to free the page.
 */
void free_page(unsigned int vpn)
{
	unsigned int pos = vpn % NR_PTES_PER_PAGE;

	struct pte *new_pte = (struct pte*)malloc(sizeof(struct pte)); 

	for (int i = 0; i < NR_PTES_PER_PAGE; i++)
	{
	    if (vpn >= i * NR_PTES_PER_PAGE && vpn < (i+1) * NR_PTES_PER_PAGE) {
		unsigned int pfn = ptbr->outer_ptes[i]->ptes[pos].pfn;

		if (mapcounts[pfn] > 1) {
		    mapcounts[pfn] -= 1;
		} else if (mapcounts[pfn] == 1){
		    mapcounts[pfn] = 0;
		}

		ptbr->outer_ptes[i]->ptes[pos] = *new_pte;
	    }
	}
}


/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	unsigned int pd_index = vpn / NR_PTES_PER_PAGE;
	unsigned int pos = vpn % NR_PTES_PER_PAGE;
	
	/* Page Table is invalid */
	if (!ptbr) {
	    alloc_page(vpn, rw);
	    return true;
	}

	struct pte_directory *pd = ptbr->outer_ptes[pd_index];

	/* Page directory does not exist */
	if (!pd) {
	    alloc_page(vpn, rw);
	    return true;
	}

	struct pte *pte = &pd->ptes[pos];

	/* PTE is invalid */
	if (!pte->valid) {
	    alloc_page(vpn, rw);
	    return true;
	}

	/* Unable to handle the write access */
	if (rw == RW_WRITE) {
	    if (!pte->writable) {
		if (pte->private == 1) {
		    if (mapcounts[pte->pfn] > 1) {
			mapcounts[pte->pfn] -= 1;
			alloc_page(vpn, rw | RW_READ);
		    } else {
			pte->writable = true;
			pte->private = 0;
		    }
		    return true;
		}
	    }
	}

	return false;
}

#include <string.h>
/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. 
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for 
 *   storing some useful information :-)
 */
void switch_process(unsigned int pid)
{ 
    struct process* p;

    // check if there is a process that we want
    list_for_each_entry(p, &processes, list) {
	if (p->pid == pid) {
	    list_add(&current->list, &processes);
	    current = p;
	    ptbr = &p->pagetable;
	    return;
	}
    }

    // fork
    struct process* new_process = (struct process*)malloc(sizeof(struct process));
    struct pagetable *new_pagetable = (struct pagetable*)malloc(sizeof(struct pagetable));

    for (int i = 0; i < NR_PTES_PER_PAGE; i++) {
        if (ptbr->outer_ptes[i]) {
	   for (int j = 0; j < NR_PTES_PER_PAGE; j++) {
	       if(ptbr->outer_ptes[i]->ptes[j].valid == true) { 
		    int pfn = ptbr->outer_ptes[i]->ptes[j].pfn;
		    mapcounts[pfn] = mapcounts[pfn] + 1;
		    if (ptbr->outer_ptes[i]->ptes[j].writable == true) {
			ptbr->outer_ptes[i]->ptes[j].writable = false;
			ptbr->outer_ptes[i]->ptes[j].private = 1; 
		   }
	       }
	    }
	}
    }

    
    for (int i = 0; i < NR_PTES_PER_PAGE; i++) {
	if (ptbr->outer_ptes[i]) {
	    struct pte_directory *new_directory = (struct pte_directory*)malloc(sizeof(struct pte_directory));
	    memcpy(new_directory, ptbr->outer_ptes[i], sizeof(struct pte_directory));
	    new_pagetable->outer_ptes[i] = new_directory;
	}
    }

    new_process->pid = pid;
    new_process->pagetable = *new_pagetable;
    list_add(&current->list, &processes); 
    
    current = new_process;
    ptbr = &current->pagetable;
}
