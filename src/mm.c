// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

 #include "mm.h"
 #include "syscall.h"
 #include "libmem.h"
 #include <stdlib.h>
 #include <stdio.h>
 
 /*
  * init_pte - Initialize PTE entry
  */
 int init_pte(uint32_t *pte,
              int pre,    // present
              int fpn,    // FPN
              int drt,    // dirty
              int swp,    // swap
              int swptyp, // swap type
              int swpoff) // swap offset
 {
   if (pre != 0)
   {
     if (swp == 0)
     { // Non swap ~ page online
       if (fpn == 0)
         return -1; // Invalid setting
 
       /* Valid setting with FPN */
       SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
       CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
       CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);
 
       SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
     }
     else
     { // page swapped
       SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
       SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
       CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);
 
       SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
       SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
     }
   }
 
   return 0;
 }
 
 /*
  * pte_set_swap - Set PTE entry for swapped page
  * @pte    : target page table entry (PTE)
  * @swptyp : swap type
  * @swpoff : swap offset
  */
 int pte_set_swap(uint32_t *pte, int swptyp, int swpoff)
 {
   CLRBIT(*pte, PAGING_PTE_PRESENT_MASK); // clear present bit
   SETBIT(*pte, PAGING_PTE_SWAPPED_MASK); // set swap bit
 
   SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT); // replace bits 0-4 of pte by swptype
   SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
 
   return 0;
 }
 
 /*
  * pte_set_swap - Set PTE entry for on-line page
  * @pte   : target page table entry (PTE)
  * @fpn   : frame page number (FPN)
  */
 int pte_set_fpn(uint32_t *pte, int fpn)
 {
   SETBIT(*pte, PAGING_PTE_PRESENT_MASK); // set present bit
   CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK); // clear swap bit
 
   SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
 
   return 0;
 }
 
 /*
  * vmap_page_range - map a range of page at aligned address
  */
 //
 int vmap_page_range(struct pcb_t *caller,           // process call
                     int addr,                       // start address which is aligned to pagesz
                     int pgnum,                      // num of mapping page
                     struct framephy_struct *frames, // list of the mapped frames
                     struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
 {                                                   // no guarantee all given pages are mapped
   // struct framephy_struct *fpit;
   int pgit = 0;
   int pgn = PAGING_PGN(addr); // addr is formated as "CPU addr scheme" which is the address in the virtual/logical addr space
 
   /* TODO: update the rg_end and rg_start of ret_rg
   //ret_rg->rg_end =  ....
   //ret_rg->rg_start = ...
   // ret_rg->vmaid = ...
   */
   ret_rg->rg_start = addr;
   ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;
 
   for (; pgit < pgnum && frames; pgit++)
   {
     pte_set_fpn(&caller->mm->pgd[pgn + pgit], frames->fpn);
     enlist_pgn_node(&caller->mm->fifo_pgn, pgn + pgit);
 
     frames = frames->fp_next;
   }
   // this means this function cannot map all page into ram
   if (pgit != pgnum)
   {
     return -1;
   }
   return 0;
   /* TODO map range of frame to address space
    *      [addr to addr + pgnum*PAGING_PAGESZ
    *      in page table caller->mm->pgd[]
    */
 
   /* Tracking for later page replacement activities (if needed)
    * Enqueue new usage page */
 }
 
 void free_frm_lst(struct framephy_struct **frm_lst, struct memphy_struct *mp)
 {
   struct framephy_struct *delFp = *frm_lst;
   while (delFp != NULL)
   {
     struct framephy_struct *next = delFp->fp_next;
     MEMPHY_put_freefp(mp, delFp->fpn);
     free(delFp);
     delFp = next;
   }
   *frm_lst = NULL;
 }
 /*
  * alloc_pages_range - allocate req_pgnum of frame in ram
  * @caller    : caller
  * @req_pgnum : request page num
  * @frm_lst   : frame list
  */
 
 /*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  
   --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  */
 
 int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
 {
   int pgit, fpn;
   struct framephy_struct *newfp_str = NULL; // pointer to a frame node
   //struct framephy_struct *prev_fp = NULL;   // pointer to the last frame node we have just link in
   /* TODO: allocate the page
   //caller-> ...
   //frm_lst-> ...
   */
 
   // oke first we neek to check maximum frames larger than req_pgnum or not
   int maxium_frames = caller->mram->maxsz / PAGING_PAGESZ;
 
   if (req_pgnum > maxium_frames)
   {
     printf("Cannot allocated due to the insufficent size of RAM");
     return -1;
   }
   for (pgit = 0; pgit < req_pgnum; pgit++)
   {
     /* TODO: allocate the page
      */
     if (MEMPHY_get_freefp(caller->mram, &fpn) == 0)
       ; // get the free frame out, and get its fpn
     else
     { // TODO: ERROR CODE of obtaining somes but not enough frames
       // swap data from busy frames to SWAP to have free frames
       int vicpgn;
       if (find_victim_page(caller->mm, &vicpgn) != 0)
       {
         // free the frm_lst
         free_frm_lst(frm_lst, caller->mram);
         return -1;
       }
       int vicfpn = PAGING_FPN(caller->mm->pgd[vicpgn]);
       int swpfpn;
       if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) != 0)
       {
         // free the frm_lst
         free_frm_lst(frm_lst, caller->mram);
         return -3000; // there is no frame left in RAM nor SWAP
       }
       struct sc_regs regs;
       regs.a1 = SYSMEM_SWP_OP;
       regs.a2 = vicfpn;
       regs.a3 = swpfpn;
       syscall(caller, 17, &regs);
       pte_set_swap(&caller->mm->pgd[vicpgn], 0, swpfpn);
       fpn = vicfpn;
     }
     // make new frame node
     newfp_str = malloc(sizeof(struct framephy_struct)); // alloc a frame node
     newfp_str->fpn = fpn;
     newfp_str->owner = caller->mm;
     newfp_str->fp_next = NULL;
     // link node to frm_lst
     if (*frm_lst == NULL)
       *frm_lst = newfp_str; // if the fisrt node, so the head would be it
     else
     {
       // prev_fp->fp_next = newfp_str; // else, link the new node to the last
       newfp_str->fp_next = *frm_lst; // or, link the new node to the first (like the output)
     }
 
     // prev_fp = newfp_str; // update the prev
     *frm_lst = newfp_str; // update frm_lst head
   }
 
   return 0;
 }
 
 /*
  * vm_map_ram - do the mapping all vm are to ram storage device
  * @caller    : caller
  * @astart    : vm area start
  * @aend      : vm area end
  * @mapstart  : start mapping point
  * @incpgnum  : number of mapped page
  * @ret_rg    : returned region
  */
 int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
 {
   struct framephy_struct *frm_lst = NULL;
   int ret_alloc;
 
 
   ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);
 
   if (ret_alloc < 0 && ret_alloc != -3000)
     return -1;
 
   /* Out of memory */
   if (ret_alloc == -3000)
   {
 #ifdef MMDBG
     printf("OOM: vm_map_ram out of memory \n");
 #endif
     return -1;
   }
   /* it leaves the case of memory is enough but half in ram, half in swap
    * do the swaping all to swapper to get the all in ram */
   vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
   
   return 0;
 }
 
 /* Swap copy content page from source frame to destination frame
  * @mpsrc  : source memphy
  * @srcfpn : source physical page number (FPN)
  * @mpdst  : destination memphy
  * @dstfpn : destination physical page number (FPN)
  **/
 int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn,
                    struct memphy_struct *mpdst, int dstfpn)
 {
   int cellidx;
   int addrsrc, addrdst;
   for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
   {
     addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
     addrdst = dstfpn * PAGING_PAGESZ + cellidx;
 
     BYTE data;
     MEMPHY_read(mpsrc, addrsrc, &data);
     MEMPHY_write(mpdst, addrdst, data);
   }
 
   return 0;
 }
 
 /*
  *Initialize a empty Memory Management instance
  * @mm:     self mm
  * @caller: mm owner
  */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));

  mm->pgd = malloc(PAGING_MAX_PGN * sizeof(uint32_t));

  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* TODO update VMA0 next */
  // vma0->next = ...
  struct vm_area_struct *vma1 = malloc(sizeof(struct vm_area_struct));
  vma1->vm_id = 1;
  vma1->vm_start = 100000;  
  vma1->vm_end = 108192  ;    
  vma1->sbrk = vma1->vm_start;  
  struct vm_rg_struct *data_rg = init_vm_rg(vma1->vm_start, vma1->vm_end);
  enlist_vm_rg_node(&vma1->vm_freerg_list, data_rg);

  vma1->vm_next = NULL;
  vma1->vm_mm = mm;

  vma0->vm_next = vma1;
  /* Point vma owner backward */
  vma0->vm_mm = mm; 

  /* TODO: update mmap */
  //mm->mmap = ...
  mm->mmap=vma0;
    for (size_t i = 0; i < PAGING_MAX_SYMTBL_SZ; i++)
   {
     mm->symrgtbl[i].rg_start = -1;
     mm->symrgtbl[i].rg_end = -1;
     mm->symrgtbl[i].rg_next = NULL;
   }
  return 0;
}
 
 struct vm_rg_struct *init_vm_rg(int rg_start, int rg_end)
 {
   struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
 
   rgnode->rg_start = rg_start;
   rgnode->rg_end = rg_end;
   rgnode->rg_next = NULL;
 
   return rgnode;
 }
 
 int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
 {
   rgnode->rg_next = *rglist;
   *rglist = rgnode;
 
   return 0;
 }
 
 int enlist_pgn_node(struct pgn_t **plist, int pgn)
 {
   struct pgn_t *pnode = malloc(sizeof(struct pgn_t));
 
   pnode->pgn = pgn;
   pnode->pg_next = *plist;
   *plist = pnode;
 
   return 0;
 }
 
 int print_list_fp(struct framephy_struct *ifp)
 {
   struct framephy_struct *fp = ifp;
 
   printf("print_list_fp: ");
   if (fp == NULL)
   {
     printf("NULL list\n");
     return -1;
   }
   printf("\n");
   while (fp != NULL)
   {
     printf("fp[%d]\n", fp->fpn);
     fp = fp->fp_next;
   }
   printf("\n");
   return 0;
 }
 
 int print_list_rg(struct vm_rg_struct *irg)
 {
   struct vm_rg_struct *rg = irg;
 
   printf("print_list_rg: ");
   if (rg == NULL)
   {
     printf("NULL list\n");
     return -1;
   }
   printf("\n");
   while (rg != NULL)
   {
     printf("rg[%ld->%ld]\n", rg->rg_start, rg->rg_end);
     rg = rg->rg_next;
   }
   printf("\n");
   return 0;
 }
 
 int print_list_vma(struct vm_area_struct *ivma)
 {
   struct vm_area_struct *vma = ivma;
 
   printf("print_list_vma: ");
   if (vma == NULL)
   {
     printf("NULL list\n");
     return -1;
   }
   printf("\n");
   while (vma != NULL)
   {
     printf("va[%ld->%ld]\n", vma->vm_start, vma->vm_end);
     vma = vma->vm_next;
   }
   printf("\n");
   return 0;
 }
 
 int print_list_pgn(struct pgn_t *ip)
 {
   printf("print_list_pgn: ");
   if (ip == NULL)
   {
     printf("NULL list\n");
     return -1;
   }
   printf("\n");
   while (ip != NULL)
   {
     printf("va[%d]-\n", ip->pgn);
     ip = ip->pg_next;
   }
   printf("n");
   return 0;
 }
 
 int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
 {
   int pgn_start, pgn_end;
   int pgit;
 
   if (end == -1)
   {
     pgn_start = 0;
     struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
     end = cur_vma->vm_end;
     // printf("%d\n", end);
   }
   pgn_start = PAGING_PGN(start);
   pgn_end = PAGING_PGN(end);
 
   printf("print_pgtbl: %d - %d", start, end);
   if (caller == NULL)
   {
     printf("NULL caller\n");
     return -1;
   }
   printf("\n");
 
   for (pgit = pgn_start; pgit < pgn_end; pgit++)
   {
     printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]); // pte is 4 bytes size, so this print the first address of each pte in pgd
   }
 
   for (pgit = pgn_start; pgit < pgn_end; pgit++)
   {
     printf("Page Number: %d -> Frame Number: %d\n", pgit, PAGING_PTE_FPN(caller->mm->pgd[pgit])); // print the page index in the pgd, and its corresponding fpn
   }
   return 0;
 }
 void dump_memory(struct mm_struct *mm) {
  // In ra thông tin vùng heap
  printf("===== Dumping memory regions =====\n");

  struct vm_area_struct *vma = mm->mmap;
  while (vma != NULL) {
      printf("VMA ID: %ld\n", vma->vm_id);
      printf("  Start Address: 0x%lx\n", vma->vm_start);
      printf("  End Address: 0x%lx\n", vma->vm_end);
      printf("  Break Address (sbrk): 0x%lx\n", vma->sbrk);
      printf("-----------------------------------\n");

      vma = vma->vm_next;  
  }
  
  // In ra thông tin về vùng heap và data
  printf("Heap and Data memory allocation:\n");
  
  struct vm_area_struct *current = mm->mmap;
  while (current != NULL) {
      
      if (current->vm_id == 0) {
          printf("Heap (vma0):\n");
          printf("  Start: 0x%lx, End: 0x%lx\n", current->vm_start, current->vm_end);
      } else if (current->vm_id == 1) {
          printf("Data (vma1):\n");
          printf("  Start: 0x%lx, End: 0x%lx\n", current->vm_start, current->vm_end);
      }
      current = current->vm_next;
  }

  printf("=====================================\n");
}
 // #endif
 