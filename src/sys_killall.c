/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */
#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "queue.h" 
#include "sched.h" 
#include <string.h> 
#include <stdlib.h> 
#include <pthread.h>

extern pthread_mutex_t queue_lock;
static struct queue_t mlq_ready_queue[MAX_PRIO];
static struct queue_t running_list;

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc_name[100];
    uint32_t data;
    uint32_t region_id = regs->a1; // Giữ lại region_id

    /* Đọc tên tiến trình từ vùng bộ nhớ */
    int i = 0;
    int read_status = 0;
    while (i < 99) { // Giới hạn buffer và kiểm tra lỗi đọc
        // Sửa: Dùng region_id thay vì region_base
        read_status = libread(caller, region_id, i, &data);

        if (read_status != 0) { // Kiểm tra return value của libread
            // printf("Error: Failed to read from memory region %d at offset %d\n", region_id, i); // Có thể bỏ comment để debug
            proc_name[i] = '\0'; // Kết thúc chuỗi nếu lỗi
            break; // Thoát vòng lặp
        }

        proc_name[i] = (char)data; // Lưu ký tự đọc được

        if (proc_name[i] == '\0') { // Kiểm tra ký tự null terminator
            break; // Thoát nếu đã đọc xong chuỗi
        }
        i++;
    }
    // Đảm bảo chuỗi luôn kết thúc bằng null, ngay cả khi đọc hết buffer
    proc_name[i] = '\0';

    // Sửa: In ra region_id thay vì region_base
    printf("The process name retrieved from memregionid %d is \"%s\"\n", region_id, proc_name);

    // char proc_name[100];
    // uint32_t data;
    // uint32_t memrg = regs->a1;
    // int i = 0;
    // do {
    //     // Sửa: Dùng memrg (region ID)
    //     if (libread(caller, memrg, i, &data) != 0) {
    //         printf("Error: Failed to read from memory region %d at offset %d\n", memrg, i);
    //          proc_name[i] = '\0';
    //          return -1;
    //     }
    //     proc_name[i] = (BYTE)data;
    //     i++;
    // } while (data != 0 && i < 100);

    // // Đảm bảo chuỗi kết thúc đúng cách
    // // Nếu vòng lặp dừng do data=0, i đã tăng thêm 1, nên vị trí cuối là i-1
    // // Nếu vòng lặp dừng do i=100, vị trí cuối là 99 (i-1)
    // if (i > 0) {
    //      proc_name[i-1] = '\0';
    // } else {
    //      proc_name[0] = '\0'; // Trường hợp không đọc được gì
    // }
    // printf("The process name retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    pthread_mutex_lock(&queue_lock); // Khóa các hàng đợi

    // --- SỬA LỖI TRUY CẬP HÀNG ĐỢI và GIẢI PHÓNG ---
    if (running_list.size > 0)
    {
        int idx = 0;
        while (idx < running_list.size)
        {
            struct pcb_t *p = running_list.proc[idx];
            if (p != NULL && strcmp(p->path, proc_name) == 0)
            {
                if (p == caller)
                {
                    p->pc = p->code->size; // Đánh dấu caller kết thúc
                    idx++; // Bỏ qua caller
                }
                else
                {
                    // Dịch chuyển xóa p khỏi running_list
                    for (int j = idx; j < running_list.size - 1; j++)
                    {
                        running_list.proc[j] = running_list.proc[j + 1];
                    }
                    running_list.proc[running_list.size - 1] = NULL;
                    running_list.size--;

                    // *** Gọi hàm giải phóng frame vật lý TRƯỚC khi free(p) ***
                    free_pcb_memph(p);
                    // *** LƯU Ý: Các tài nguyên khác (mm, pgd) CẦN được giải phóng ở nơi khác (ví dụ trong free_pcb_memph)! ***
                    free(p); // Giải phóng PCB

                    // Không tăng idx vì phần tử tiếp theo đã dịch lên
                }
            }
            else
            {
                idx++; // Chuyển sang phần tử tiếp theo
            }
        }
    }

#ifdef MLQ_SCHED
    for (int prio = 0; prio < MAX_PRIO; prio++)
    {
        struct queue_t *q = &mlq_ready_queue[prio];
        if (q->size > 0)
        {
            int idx = 0;
            while (idx < q->size)
            {
                struct pcb_t *p = q->proc[idx];
                if (p != NULL && strcmp(p->path, proc_name) == 0)
                {
                    // Dịch chuyển xóa p khỏi hàng đợi q
                    for (int j = idx; j < q->size - 1; j++)
                    {
                        q->proc[j] = q->proc[j + 1];
                    }
                    q->proc[q->size - 1] = NULL;
                    q->size--;

                    // Không kill caller ở đây vì nó đang chạy syscall
                    if (p != caller) {
                         // *** SỬA LỖI: Gọi hàm giải phóng frame vật lý TRƯỚC khi free(p) ***
                         free_pcb_memph(p);
                         // *** LƯU Ý: Các tài nguyên khác (mm, pgd) CẦN được giải phóng ở nơi khác! ***
                         free(p); // Giải phóng PCB
                    }
                    // Không tăng idx
                }
                else
                {
                    idx++; // Chuyển sang phần tử tiếp theo
                }
            }
        }
    }
#endif
    // --- Kết thúc sửa lỗi truy cập và giải phóng ---

    pthread_mutex_unlock(&queue_lock); // Mở khóa

    return 0;
}
