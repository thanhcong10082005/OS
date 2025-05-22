Lưu ý quan trọng:  
  Để chạy được các file test sched, sched_0, sched_1 của scheduler thì phải vào include/os-cfg.h và uncomment 3 dòng sau:  
// #define MM_FIXED_MEMSZ  
// #define VMDBG 1  
// #define MMDBG 1  
Các Bước Làm Việc :
1. Lấy Code về máy :  
  > mở Terminal/Git Bash, vào thư mục muốn lưu code, và chạy:  
    git clone https://github.com/thanhcong10082005/OS  
    cd OS
2. Tạo Nhánh Riêng cho Phần việc của Mình:  
 > Đảm bảo đang ở nhánh main và đã cập nhật code mới nhất:  
   git checkout main  
    git pull origin main  
  Tạo và chuyển sang nhánh mới (ví dụ, bạn A làm phần feature-A):  
   git checkout -b feature/tudat  
    tudat : là tự m đặt  
3. Làm việc và Commit trên Nhánh Riêng:  
 > Sau khi hoàn thành một chức năng, commit:  
    git add . # Hoặc git add <file-cu-the> để thêm tất cả các file vào staging area  
    git commit -m "M đã làm được gì"  
4. Đẩy Nhánh Riêng lên GitHub:  
>  git push -u origin feature/tudat  
5. Tạo Pull Request (PR) trên GitHub:  
>  vào tab "Pull requests" -> "New pull request", chọn base: main và compare: feature/tudat  
6. Merge PR  
>  Sau khi code đã ổn, nhấn "Merge pull request" để tích hợp code từ nhánh feature vào nhánh main.  

