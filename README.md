Các Bước Làm Việc :
Bước 1: Lấy Code về máy :
  mở Terminal/Git Bash, vào thư mục muốn lưu code, và chạy:
    git clone https://github.com/thanhcong10082005/OS
    cd OS
Bước 2: Tạo Nhánh Riêng cho Phần việc của Mình:
  1. Đảm bảo đang ở nhánh main và đã cập nhật code mới nhất:
    git checkout main
    git pull origin main
  2. Tạo và chuyển sang nhánh mới (ví dụ, bạn A làm phần feature-A):
    git checkout -b feature/tudat
    tudat : là tự m đặt
Bước 3: Làm việc và Commit trên Nhánh Riêng:
  Sau khi hoàn thành một chức năng, commit:
    git add . # Hoặc git add <file-cu-the> để thêm tất cả các file vào staging area
    git commit -m "M đã làm được gì"
Bước 4: Đẩy Nhánh Riêng lên GitHub:
  git push -u origin feature/tudat
Bước 5: Tạo Pull Request (PR) trên GitHub:
  vào tab "Pull requests" -> "New pull request".
  Chọn base: main và compare: feature/tudat
Bước 6: Merge PR
  Sau khi code đã ổn, nhấn "Merge pull request" để tích hợp code từ nhánh feature vào nhánh main.

