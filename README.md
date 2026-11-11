# MAPF: CBS và ECBS

Bài tập lớn môn Advanced Algorithms, Chủ đề 9: Multi-Agent Path Finding tối ưu và bounded-suboptimal. Nhóm tự cài Conflict-Based Search (CBS) và Enhanced CBS (ECBS) bằng C++17, và dùng Python cho benchmark MovingAI, thống kê và hình vẽ.

Mô hình bài toán, các loại xung đột, hàm mục tiêu và định dạng dữ liệu được cố định trong [docs/mapf-spec.md](docs/mapf-spec.md). Mọi thành phần phải tuân theo tài liệu đó.

## Trạng thái

Repo đang ở tuần 1. Đã có, kèm unit test:

- Khung CMake, các kiểu dữ liệu dùng chung, lưới 4 hướng và bộ đọc file MovingAI.
- Tầng thấp trong `src/lowlevel/`: `HeuristicTable` (khoảng cách BFS tới đích), `ConstraintTable` (ràng buộc của một agent) và `find_path` (Space-Time A* tối ưu). `find_path` được đối chiếu với bộ tìm kiếm tham chiếu trong `tests/reference_search.h` trên các instance ngẫu nhiên có seed cố định.

Chưa có CBS, ECBS và executable; target `mapf_solver` được thêm cùng `src/app/main.cpp` khi CBS chạy được.

## Yêu cầu

- CMake 3.20 trở lên.
- Trình biên dịch 64-bit hỗ trợ C++17.
- Ninja hoặc một generator khác của CMake.

Không dùng bản GCC 32-bit: tiến trình 32-bit chỉ có khoảng 2 GB bộ nhớ, không đủ cho CBS trên instance khó.

### Cài trên Windows bằng MSYS2 UCRT64

1. Cài MSYS2 từ <https://www.msys2.org/> hoặc bằng `winget install MSYS2.MSYS2`. Thư mục mặc định là `C:\msys64`.
2. Mở shell **MSYS2 UCRT64** và chạy lần lượt:

   ```bash
   pacman -Syu
   ```

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```

   Nếu `pacman -Syu` đóng shell sau khi cập nhật, mở lại shell và chạy lệnh đó thêm một lần.
3. Để build từ PowerShell hoặc từ IDE, thêm `C:\msys64\ucrt64\bin` vào biến môi trường PATH, đặt trước mọi bản MinGW cũ.
4. Kiểm tra: `g++ -dumpmachine` phải in ra `x86_64-w64-mingw32`.

## Build và chạy test

Chạy từ thư mục gốc của repo:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build
```

```bash
ctest --test-dir build --output-on-failure
```

Build mặc định là Release. Trên MinGW, các file thực thi được link tĩnh nên chạy được mà không cần DLL của compiler trên PATH.

## Cấu trúc thư mục

| Đường dẫn | Nội dung | Phụ trách |
|-----------|----------|-----------|
| `CMakeLists.txt` | Cấu hình build | TV1 |
| `docs/mapf-spec.md` | Đặc tả mô hình MAPF | TV1 |
| `src/core/` | `types.h`, lưới, bộ đọc instance | TV1 |
| `src/lowlevel/` | Bảng ràng buộc, heuristic, Space-Time A*, MDD | TV1 |
| `src/cbs/` | CBS; `ct_node.h` | TV1; TV2 |
| `src/ecbs/` | ECBS và FOCAL | TV2 |
| `src/conflict/` | Phát hiện và chọn xung đột | TV3 |
| `src/validation/` | Validator độc lập, oracle | TV4 |
| `src/app/` | Executable `mapf_solver` | TV1 |
| `tests/` | Unit test; mỗi người viết test cho phần mình | Cả nhóm |
| `python/` | Runner, manifest, thống kê, hình | TV3, TV4 |
| `third_party/` | Thư viện bên ngoài | TV1 |

Hiện mới có `src/core/`, `src/lowlevel/`, `tests/`, `docs/` và `third_party/`. Các thư mục còn lại được tạo khi có code.

Quy ước: tên file C++ dùng snake_case, mã nguồn nằm trong namespace `mapf`, và include tính từ `src/` (ví dụ `#include "core/grid.h"`).

## Trình biên dịch đã kiểm tra

Số liệu benchmark chỉ so sánh được khi cùng compiler và cùng cờ build. Mỗi thành viên ghi máy của mình vào bảng sau khi build và chạy `ctest` thành công.

| Máy | Hệ điều hành | Compiler | CMake | Generator | Ngày kiểm tra |
|-----|--------------|----------|-------|-----------|---------------|
| TV1 | Windows 11 Home (10.0.26200) | GCC 16.2.0, MSYS2 UCRT64, `x86_64-w64-mingw32` | 4.4.5 | Ninja 1.13.2 | 2026-11-11 |

## Thư viện bên ngoài

| Thư viện | Phiên bản | License | Nguồn | Dùng cho |
|----------|-----------|---------|-------|----------|
| doctest | 2.5.3 | MIT | <https://github.com/doctest/doctest> | Unit test |

`third_party/doctest/doctest.h` và `LICENSE.txt` là bản sao nguyên vẹn từ tag `v2.5.3`. Lõi thuật toán do nhóm tự viết.

## Dữ liệu benchmark trong repo

`tests/data/movingai/` chứa hai file nguyên vẹn từ bộ MAPF của MovingAI (<https://movingai.com/benchmarks/mapf/index.html>), tải ngày 2026-11-11, chỉ dùng cho unit test của bộ đọc instance:

- `random-32-32-20.map`, từ `random-32-32-20.map.zip`.
- `random-32-32-20-random-1.scen`, từ `random-32-32-20.map-scen-random.zip`.

Dữ liệu cho benchmark chính không nằm trong repo; runner tải theo manifest.
