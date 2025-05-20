/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c
 */

 #include "string.h"
 #include "mm.h"
 #include "syscall.h"
 #include "libmem.h"
 #include <stdlib.h>
 #include <stdio.h>
 #include <pthread.h>
 
 //static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
 
 /*enlist_vm_freerg_list - add new rg to freerg_list
  *@mm: memory region
  *@rg_elmt: new region
  *
  */
 int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
 {
   struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
 
   if (rg_elmt->rg_start >= rg_elmt->rg_end)
     return -1;
 
   if (rg_node != NULL)
     rg_elmt->rg_next = rg_node;
 
   /* Enlist the new region */
   mm->mmap->vm_freerg_list = rg_elmt;
 
   return 0;
 }
 
 /*get_symrg_byid - get mem region by region ID
  *@mm: memory region
  *@rgid: region ID act as symbol index of variable
  *
  */
 struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
 {
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return NULL;
 
   return &mm->symrgtbl[rgid];
 }
 
 /*__alloc - allocate a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *@alloc_addr: address of allocated memory region
  *
  */
 int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
 {
   /*Allocate at the toproof */
   struct vm_rg_struct rgnode;
   //int inc_limit_ret; // new sbrk
 
   /* TODO: commit the vmaid */
   // rgnode.vmaid
   rgnode.vmaid = vmaid;
 
   if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
   {
     caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
     *alloc_addr = rgnode.rg_start;
     // DEBUG:
     //printf("get free area success\n");
 
     return 0;
   }
 
   /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
   else
   {
     // DEBUG:
     //  printf("cannot get free area\n");
     struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
     //printf("Hello");
     //printf("SBRK: %d  END: %d", cur_vma->sbrk, cur_vma->vm_end);
 
     // @Nhan: Be careful when using aligned size (inc_sz) to extend sbrk.
     // Since inc_sz is rounded up to the nearest page size, sbrk may hit vm_end
     // sooner than expected, even if there’s still technically some space left.
     // This isn't wrong, but it wastes memory — and over time, we might run out of it.
     // So in practice, aligning everything may not be the best idea.
 
     int inc_sz = PAGING_PAGE_ALIGNSZ(size);
 
     if (cur_vma->vm_end - cur_vma->sbrk + 1 < size)
     {
       //int old_sbrk = cur_vma->sbrk;
       struct sc_regs regs;
       regs.a1 = SYSMEM_INC_OP;
       regs.a2 = vmaid;
       regs.a3 = inc_sz;
 
       syscall(caller, 17, &regs);
       // alloc_addr = &old_sbrk;
       // if (get_free_vmrg_area(caller, vmaid, size, &rgnode) != 0)
       // {
       //   pthread_mutex_unlock(&mmvm_lock);
       //   return -1;
       // }
       // DEBUG:
       // printf("print pgd in alloc 3: ");
       // print_pgtbl(caller, 0, -1); // In page table
 
       rgnode = *get_vm_area_node_at_brk(caller, vmaid, size, size);
       caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
       caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
       *alloc_addr = rgnode.rg_start;
       // printf("addr in alloc 2: ");
       // printf("addr: %08x\n", *alloc_addr);
       cur_vma->sbrk = rgnode.rg_end;
       //inc_limit_ret = cur_vma->sbrk;
       return 0;
     }
 
     // printf("addr in alloc 1: ");
     // printf("addr: %08x\n", *alloc_addr);
     rgnode = *get_vm_area_node_at_brk(caller, vmaid, size, size); 
     caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
     *alloc_addr = rgnode.rg_start;
     // printf("addr in alloc 2: ");
     // printf("addr: %08x\n", *alloc_addr);
     cur_vma->sbrk = rgnode.rg_end;
     //inc_limit_ret = cur_vma->sbrk;
     // DEBUG:
     // printf("print pgd in alloc 4: ");
     // print_pgtbl(caller, 0, -1); // In page table
   }
 
   return 0;
 }
 
 /*__free - remove a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __free(struct pcb_t *caller, int vmaid, int rgid)
 {
   //struct vm_rg_struct rgnode;
 
   // Dummy initialization for avoding compiler dummay warning
   // in incompleted TODO code rgnode will overwrite through implementing
   // the manipulation of rgid later
 
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return -1;
 
   /* TODO: Manage the collect freed region to freerg_list */
   struct vm_rg_struct *free_region = get_symrg_byid(caller->mm, rgid);
   int free_start = free_region->rg_start;
   int free_end = free_region->rg_end;
 
   // freed free_region
   free_region->rg_start = -1;
   free_region->rg_end = -1;
 
   struct vm_rg_struct *new_free_area = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));
   new_free_area->rg_start = free_start;
   new_free_area->rg_end = free_end;
   new_free_area->vmaid = vmaid;
   /*enlist the obsoleted memory region */
   enlist_vm_freerg_list(caller->mm, new_free_area);
 
   return 0;
 }
 
 /*liballoc - PAGING-based allocate a region memory
  *@proc:  Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
 {
   // DEBUG:
   // printf("first print: ");
   // print_pgtbl(proc, 0, -1);
 
   int addr = 0;
   int freerg_id = -1;    // the region id that has not been in logical address, so also not point to a frame
   //int freerg_vmaid = -1; //  we also need to update freerg_vmaid to use _alloc()
   if (proc->mm->symrgtbl[reg_index].rg_start == -1 &&
       proc->mm->symrgtbl[reg_index].rg_end == -1)
   {
     // printf("use reg_index\n"); DEBUG
     freerg_id = reg_index;
     //freerg_vmaid = proc->mm->symrgtbl[reg_index].vmaid;
   }
   else
   {
     for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++)
     {
       if (proc->mm->symrgtbl[i].rg_start == -1 &&
           proc->mm->symrgtbl[i].rg_end == -1)
       {
         freerg_id = i;
         //freerg_vmaid = proc->mm->symrgtbl[i].vmaid;
         break;
       }
     }
   }
 
   if (freerg_id == -1)
     return -1;
   // DEBUG:
   // printf("2 print: ");
   // print_pgtbl(proc, 0, -1); // In page table
   // printf("addr in liballoc 1: %08x\n", addr);
   if (__alloc(proc, 0, freerg_id, size, &addr) == 0)
   {
     // proc->regs[reg_index] = addr;
     proc->regs[freerg_id] = addr;
   }
   // printf("addr in liballoc 2: %08x\n", addr);
   // DEBUG:
   // printf("3 print: ");
   // print_pgtbl(proc, 0, -1); // In page table
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
   printf("PID=%d - Region=%d - Address=%08x - Size=%u byte\n", proc->pid, freerg_id, addr, size);
 #ifdef PAGETBL_DUMP
 
   print_pgtbl(proc, 0, -1); // In page table
 #endif
   // MEMPHY_dump(proc->mram); // In nội dung RAM
   printf("================================================================\n");
 #endif
   return 0;
 }
 
 /*libfree - PAGING-based free a region memory
  *@proc: Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
   uint32_t region_id = -1;
   //uint32_t region_vmaid = -1; //  we also need to get region_vmaid to use _free()
   if (reg_index >= PAGING_MAX_SYMTBL_SZ)
     return -1;
 
   if (proc->mm->symrgtbl[reg_index].rg_start == proc->regs[reg_index])
   {
     region_id = reg_index;
     //region_vmaid = proc->mm->symrgtbl[reg_index].vmaid;
   }
   else
   {
     for (size_t i = 0; i < PAGING_MAX_SYMTBL_SZ; i++)
     {
       if (proc->mm->symrgtbl[i].rg_start == proc->regs[reg_index] &&
           proc->mm->symrgtbl[i].rg_end > proc->mm->symrgtbl[i].rg_start)
       {
         region_id = i;
         //region_vmaid = proc->mm->symrgtbl[i].vmaid;
         break;
       }
     }
   }
 
   if (region_id == -1)
     return -1;
 
   int result = __free(proc, 0, region_id);
 
   if (result == 0)
   {
     proc->regs[reg_index] = -1;
 
 #ifdef IODUMP
     printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
     /*
     printf("PID=%d - Region=%d \n",
            proc->pid,
            region_id,
            proc->mm->symrgtbl[region_id].rg_start,
            proc->mm->symrgtbl[region_id].rg_end);
     */
    printf("PID=%d - Region=%d\n", proc->pid, region_id);
 #ifdef PAGETBL_DUMP
     print_pgtbl(proc, 0, -1);
 #endif
     // MEMPHY_dump(proc->mram);
 #endif
   }
 
   return result;
 }
 
 /*pg_getpage - get the page in ram
  *@mm: memory region
  *@pagenum: PGN
  *@framenum: return FPN
  *@caller: caller
  *
  */
 int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
 {
   // first of all we need to check the page present or not
   uint32_t pte = mm->pgd[pgn];
   if (PAGING_PAGE_PRESENT(pte))
   {
     *fpn = PAGING_FPN(pte);
   }
   else
   {
     // it means the page  dont have the physical memory in ram
     //  I consider two cases happen, first the physcical memory had not been allocated yet.
     //  So we need to check if ram have enough space for allocation, we will do it
     //  othrerwise if ram have no space, we will SWAP for swap in and swap out
     int vicpgn, vicfpn, swpfpn;
     int tgtfpn = PAGING_PTE_SWP(pte);
     int freefpn;
     if (MEMPHY_get_freefp(caller->mram, &freefpn) != -1)
     {
       __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, freefpn);
       pte_set_fpn(&mm->pgd[pgn], freefpn);
       // pte_set_present(&mm->pgd[pgn]);
       *fpn = freefpn;
       enlist_pgn_node(&mm->fifo_pgn, pgn); // 💥 add to FIFO after allocation
       printf("Have enough space for allocation!");
     }
     // this means when we dont have enough memory for allocation!
     else
     {
       // first of all we need to find where exactly our pages is
       // We know that in the structure of pte when swapped = 1
       // we have the swptype and swpoffset
       // which actualy means swptype is the region, and swpoffset is actual index in that region
       // because the swap ad ram have same struct memphy ....
       if (find_victim_page(caller->mm, &vicpgn) < 0)
       {
         return -1;
       }
 
       vicfpn = PAGING_FPN(mm->pgd[vicpgn]);
       if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0)
       {
         printf("There is no space in swap\n"); // for debuging
         return -1;
       }
       // swap the victim fpn to the swp at the location swpfpn
       __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
       __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
       
       // after swap we need to modify the pte to let the page table know that the vicfpn is not actualy in ram
       //  but it has been stored in swp
       // about the swptype i could be wrong, the right way is determine exactly what page number of swpfpn;
       // but im too lazy for figuring out this :)).
       pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);
       // pte_clear_present(&mm->pgd[vicpgn]);
 
       pte_set_fpn(&mm->pgd[pgn], vicfpn);
       // pte_set_present(&mm->pgd[pgn]);
       *fpn = vicfpn;
       enlist_pgn_node(&mm->fifo_pgn, pgn); // 💥 add to FIFO after swap-in
     }
   }
 
   return 0;
 }
 
 /*pg_getval - read value at given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
     //int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
   /* TODO
    *  MEMPHY_read(caller->mram, phyaddr, data);
    *  MEMPHY READ
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_READ
    */
   int phyaddr = (fpn * PAGE_SIZE) + off;
   struct sc_regs regs;
   regs.a1 = SYSMEM_IO_READ;
   regs.a2 = phyaddr;
   // regs.a3 = *data; // we dont need a3 for SYSTEM_IO_READ
 
   /* SYSCALL 17 sys_memmap */
   syscall(caller, 17, &regs);
   // Update data
   *data = regs.a3;
 
   return 0;
 }
 
 /*pg_setval - write value to given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
 
   /* TODO
    *  MEMPHY_write(caller->mram, phyaddr, value);
    *  MEMPHY WRITE
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_WRITE
    */
   int phyaddr = (fpn * PAGE_SIZE) + off;
   struct sc_regs regs;
   regs.a1 = SYSMEM_IO_WRITE;
   regs.a2 = phyaddr;
   regs.a3 = value;
   syscall(caller, 17, &regs);
   /* SYSCALL 17 sys_memmap */
 
   // Update data
   // data = (BYTE)
 
   return 0;
 }
 
 /*__read - read value in region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_getval(caller->mm, currg->rg_start + offset, data, caller);
 
   return 0;
 }
 
 /*libread - PAGING-based read a region memory */
 int libread(
     struct pcb_t *proc, // Process executing the instruction
     uint32_t source,    // Index of source register
     uint32_t offset,    // Source address = [source] + [offset]
     uint32_t *destination)
 {
   BYTE data;
   int val = __read(proc, 0, source, offset, &data);
 
   /* TODO update result of reading action*/
   *destination = (uint32_t)data;
 
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER READING =====\n");
   printf("read region=%d offset=%d value=%d\n", source, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   // MEMPHY_dump(proc->mram);
   printf("===== PHYSICAL MEMORY DUMP =====\n");
   MEMPHY_dump(proc->mram); // Bỏ comment dòng này
   printf("===== PHYSICAL MEMORY END-DUMP =====\n");
   printf("================================================================\n");
 #endif
 
   return val;
 }
 
 /*__write - write a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_setval(caller->mm, currg->rg_start + offset, value, caller);
 
   return 0;
 }
 
 /*libwrite - PAGING-based write a region memory */
 int libwrite(
     struct pcb_t *proc,   // Process executing the instruction
     BYTE data,            // Data to be wrttien into memory
     uint32_t destination, // Index of destination register
     uint32_t offset)
 {
   int val = __write(proc, 0, destination, offset, data);
   if (val == -1)
   {
     return -1;
   }
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   // MEMPHY_dump(proc->mram);
   printf("===== PHYSICAL MEMORY DUMP =====\n");
   MEMPHY_dump(proc->mram); // Bỏ comment dòng này
   printf("===== PHYSICAL MEMORY END-DUMP =====\n");
   printf("================================================================\n");
 #endif
 
   return __write(proc, 0, destination, offset, data);
   printf("================================================================\n");
 }
 
 /*free_pcb_memphy - collect all memphy of pcb
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@incpgnum: number of page
  */
 int free_pcb_memph(struct pcb_t *caller)
 {
   int pagenum, fpn;
   uint32_t pte;
 
   for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
   {
     pte = caller->mm->pgd[pagenum];
 
     if (!PAGING_PAGE_PRESENT(pte))
     {
       fpn = PAGING_PTE_FPN(pte);
       MEMPHY_put_freefp(caller->mram, fpn);
     }
     else
     {
       fpn = PAGING_PTE_SWP(pte);
       MEMPHY_put_freefp(caller->active_mswp, fpn);
     }
   }
 
   return 0;
 }
 
 /*find_victim_page - find victim page
  *@caller: caller
  *@pgn: return page number
  *
  */
 int find_victim_page(struct mm_struct *mm, int *retpgn)
 {
   struct pgn_t *pg = mm->fifo_pgn;
 
   /* TODO: Implement the theorical mechanism to find the victim page */
   if (!pg)
   {
     return -1;
   }
   struct pgn_t *prev = NULL;
   while (pg->pg_next)
   {
     prev = pg;
     pg = pg->pg_next;
   }
   *retpgn = pg->pgn;
   prev->pg_next = NULL;
   free(pg);
 
   return 0;
 }
 
 /*get_free_vmrg_area - get a free vm region
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@size: allocated size
  *
  */
 int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
 {
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list; // dia chia dau cua current
 
   if (rgit == NULL)
     return -1;
 
   /* Probe unintialized newrg */
   struct vm_rg_struct *temp = NULL;
   newrg->rg_start = newrg->rg_end = -1;
   /* TODO Traverse on list of free vm region to find a fit space */
   // while (...)
   // freelist-> [0..9] -> [12 ...22]
   // [0->3]
   while (rgit != NULL)
   {
     if (rgit->rg_end - rgit->rg_start + 1 >= size)
     {
       newrg->rg_start = rgit->rg_start;
       newrg->rg_end = rgit->rg_start + size - 1;
 
       rgit->rg_start = newrg->rg_end + 1;
 
       if (rgit->rg_start > rgit->rg_end)
       {
         if (temp == NULL) // firstnode
         {
           struct vm_rg_struct *del = rgit;
           rgit = rgit->rg_next;
           free(del);
         }
         else
         {
           struct vm_rg_struct *del = temp->rg_next;
           temp->rg_next = rgit->rg_next;
           free(del);
         }
       }
       return 0;
     }
     temp = rgit;
     rgit = rgit->rg_next;
   }
   return -1;
 }
 
 // #endif
 