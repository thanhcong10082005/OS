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
 
 static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
 
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

  /* TODO: commit the vmaid */
  // rgnode.vmaid

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
    *alloc_addr = rgnode.rg_start;
     pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);   
  int old_sbrk = cur_vma->sbrk;
  //int new_sbrk = old_sbrk + inc_sz;
  
  struct sc_regs regs;
  regs.a1 = SYSMEM_INC_OP;
  regs.a2 = vmaid;
  regs.a3 = inc_sz;
  regs.orig_ax=17;
  if(syscall(caller,regs.orig_ax, &regs)<0) {
    return -1; 
  }
  
  
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk+size;

  
  *alloc_addr = old_sbrk;
  cur_vma->sbrk = cur_vma->vm_end;
  if (old_sbrk+size < cur_vma->vm_end)
  {
    struct vm_rg_struct *rg_free = malloc(sizeof(struct vm_rg_struct));
    rg_free->rg_start = old_sbrk+size;
    rg_free->rg_end = cur_vma->vm_end;
    enlist_vm_freerg_list(caller->mm, rg_free);
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

  pthread_mutex_lock(&mmvm_lock);

   int addr = 0;
   int freerg_id = -1;    // the region id that has not been in logical address, so also not point to a frame
  
   if (proc->mm->symrgtbl[reg_index].rg_start == -1 &&
       proc->mm->symrgtbl[reg_index].rg_end == -1)
   {
    
     freerg_id = reg_index;
     
   }
   else
   {
    return -1;
    pthread_mutex_unlock(&mmvm_lock);
   }
 
 

   if (__alloc(proc, 0, freerg_id, size, &addr) == 0)
   {
    
     proc->regs[reg_index] = addr;
   }

 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
   printf("PID=%d - Region=%d - Address=%08x - Size=%u byte\n", proc->pid, freerg_id, addr, size);
 #ifdef PAGETBL_DUMP
 
   print_pgtbl(proc, 0, -1); 
 #endif
 
   printf("================================================================\n");
 #endif
    pthread_mutex_unlock(&mmvm_lock);
 
   return 0;
 }
 
 /*libfree - PAGING-based free a region memory
  *@proc: Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 int libfree(struct pcb_t *proc, uint32_t reg_index)
 { 
      
    pthread_mutex_lock(&mmvm_lock);
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
    printf("================================================================\n");

     // MEMPHY_dump(proc->mram);
 #endif
 pthread_mutex_unlock(&mmvm_lock);
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
  uint32_t pte = mm->pgd[pgn];
  
  if (!PAGING_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */
    int vicpgn, swpfpn; 
    int vicfpn, swp_id;
    //uint32_t vicpte;

    int tgtfpn = PAGING_PTE_SWP(pte);//the target frame storing our variable
    
    int typ_swp=PAGING_PTE_SWPTYP(pte);
    if(caller->mswp[typ_swp]->maxsz<256) typ_swp=caller->active_mswp_id;
    /* TODO: Play with your paging theory here */
    /* Find victim page */
    if(find_victim_page(caller->mm, &vicpgn)!=0) {
      return -1;
    }
    /* Get free frame in MEMSWP */
    if(SWAPMEM_try_get_freefp(caller, &swpfpn,&swp_id)!=0) {
      return -1;
    }

    //if(MEMPHY_get_freefp(caller->active_mswp, &swpfpn)!=0) return -1;

    /* TODO: Implement swap frame from MEMRAM to MEMSWP and vice versa*/
    vicfpn = PAGING_PTE_FPN(mm->pgd[vicpgn]);
    
    struct sc_regs regs;
    regs.a1 = SYSMEM_SWP_OP;
    regs.a2 = vicfpn;
    regs.a3 = swpfpn;
    //regs.a4= 0;
    //regs.a5=swp_id;
    //regs.orig_ax=17;
    //if(PAGING_PAGE_DIRTY(mm->pgd[vicpgn]))
      syscall(caller,regs.orig_ax, &regs);
    //__swap_cp_page(caller->mram, vicfpn, caller->mswp[swp_id], swpfpn);
    regs.a2 = tgtfpn;
    regs.a3 = vicfpn;
    //regs.a4= 1;
    //regs.a5=typ_swp;
    syscall(caller,regs.orig_ax, &regs);
    //__swap_cp_page(caller->mswp[typ_swp], tgtfpn, caller->mram, vicfpn);
    pte_set_swap(&mm->pgd[vicpgn], swp_id, swpfpn);
    pte_set_fpn(&mm->pgd[pgn], vicfpn);
    enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
  }
  *fpn = PAGING_FPN(mm->pgd[pgn]);
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
  uint32_t phyaddr=(fpn << PAGING_ADDR_FPN_LOBIT) | off;
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
   SETBIT(mm->pgd[pgn], PAGING_PTE_DIRTY_MASK);
  struct sc_regs regs;
  uint32_t addr_phy=(fpn << (PAGING_ADDR_FPN_LOBIT)) | off;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2= addr_phy;
  regs.a3 = value;
  regs.orig_ax=17;
  if(syscall(caller,regs.orig_ax, &regs)<0) return -1;
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
  pthread_mutex_lock(&mmvm_lock);
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
    pthread_mutex_unlock(&mmvm_lock);

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
   pthread_mutex_lock(&mmvm_lock);  
   int val = __write(proc, 0, destination, offset, data);
   if (val == -1)
   {
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
    MEMPHY_dump(proc->mram);
   printf("===== PHYSICAL MEMORY DUMP =====\n");
   MEMPHY_dump(proc->mram); // Bỏ comment dòng này
   printf("===== PHYSICAL MEMORY END-DUMP =====\n");
   printf("================================================================\n");
 #endif
    pthread_mutex_unlock(&mmvm_lock);
   return val;
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
 