# mapf-cbs-core

Lõi C++17 cho bài toán Multi-Agent Path Finding (MAPF) tối ưu theo sum-of-costs: mô hình bài toán, bộ đọc benchmark MovingAI, tìm kiếm tầng thấp Space-Time A\* và executable `mapf_solver`. Đây là một module của bài tập lớn môn Advanced Algorithms, Chủ đề 9 (MAPF: CBS, ECBS, EECBS), và được gắn vào repo chính của nhóm dưới dạng git submodule.

Mô hình bài toán, các loại xung đột, hàm mục tiêu và định dạng dữ liệu được cố định trong [docs/mapf-spec.md](docs/mapf-spec.md). Mọi thành phần phải tuân theo tài liệu đó.

## Trạng thái

Đã có, kèm unit test:

- Các kiểu dữ liệu dùng chung, lưới 4 hướng và bộ đọc file `.map`, `.scen` của MovingAI.
- Tầng thấp trong `src/lowlevel/`:
  - `HeuristicTable`: khoảng cách BFS từ mọi ô tới đích.
  - `ConstraintTable`: các ràng buộc vertex và edge của một agent.
  - `find_path`: Space-Time A\* tối ưu, luôn dừng kể cả khi vô nghiệm. Được đối chiếu với bộ tìm kiếm tham chiếu `tests/reference_search.h` trên các instance ngẫu nhiên có seed cố định.
  - `build_mdd`: MDD của một agent, dùng để phân loại cardinal conflict.
- Executable `mapf_solver`: đọc tham số, nạp instance, tính cận dưới độc lập và ghi kết quả JSON.

Chưa có CBS và ECBS. Cho tới khi có, `mapf_solver` ghi `status: "error"` với lý do solver chưa có trong bản build. Giao diện dòng lệnh, mã thoát và các khoá JSON đã cố định để runner tích hợp trước.

## Yêu cầu

- CMake 3.21 trở lên.
- Trình biên dịch 64-bit hỗ trợ C++17 (GCC, Clang hoặc MSVC).
- Ninja hoặc một generator khác của CMake.
- Python 3 (không bắt buộc) để chạy test đầu-cuối của `mapf_solver`.

Không dùng bản GCC 32-bit: tiến trình 32-bit chỉ có khoảng 2 GB bộ nhớ, không đủ cho CBS trên instance khó.

Trên Linux, cài gói `build-essential`, `cmake` và `ninja-build`; trên macOS, cài Xcode Command Line Tools rồi `brew install cmake ninja`.

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
3. Để build từ PowerShell hoặc IDE, thêm `C:\msys64\ucrt64\bin` vào PATH, đặt trước mọi bản MinGW cũ.
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

Khi build độc lập, build type mặc định là Release. Trên MinGW, các file thực thi được link tĩnh nên chạy được mà không cần DLL của compiler trên PATH. Nếu có Python 3, `ctest` chạy thêm `tests/check-solver-cli.py` để kiểm tra `mapf_solver` từ đầu đến cuối.

## Dùng như submodule

Trong repo chính:

```bash
git submodule add <url-of-this-repo> mapf-cbs-core
```

Rồi trong `CMakeLists.txt` của repo chính:

```cmake
add_subdirectory(mapf-cbs-core)
target_link_libraries(<target> PRIVATE mapf_core)
```

Khi được nhúng như vậy, repo này chỉ cung cấp thư viện `mapf_core` và executable `mapf_solver`. Unit test và benchmark chỉ build khi repo được build độc lập, hoặc khi bật `-DMAPF_CORE_BUILD_TESTS=ON`. Build type và cờ biên dịch do repo chính quyết định.

## Chạy solver

```bash
build/mapf_solver --map tests/data/tiny.map --scen tests/data/tiny.scen --agents 3 --solver cbs --output result.json
```

`build/mapf_solver --help` liệt kê mọi tham số. Instance mức `k` agent lấy `k` dòng đầu của file `.scen`.

| Mã thoát | Ý nghĩa |
|----------|---------|
| 0 | Đã ghi file kết quả; xem khoá `status` (`solved`, `timeout`, `no_solution`, `error`, `invalid`) |
| 1 | Không nạp được map hoặc scenario; file kết quả vẫn được ghi với `status: "error"` |
| 2 | Sai tham số dòng lệnh; không ghi file kết quả |
| 3 | Không ghi được file kết quả |

File kết quả là một đối tượng JSON có `schema_version` 1. Danh sách khoá và thứ tự nằm trong `tests/check-solver-cli.py`. Với `--paths <file>`, lời giải được ghi thêm dưới dạng text: dòng `agents <k>`, rồi mỗi agent một dòng gồm chỉ số và các cặp `x,y` theo từng thời điểm.

## Đo tốc độ tầng thấp

```bash
cmake --build build --target bench_lowlevel
```

Lệnh này chạy `find_path` cho 409 agent trên `random-32-32-20` trong bốn tình huống ràng buộc, rồi in thời gian và số trạng thái mỗi lần gọi.

## Cấu trúc thư mục

| Đường dẫn | Nội dung |
|-----------|----------|
| `docs/mapf-spec.md` | Đặc tả mô hình MAPF |
| `src/core/` | Kiểu dữ liệu dùng chung, lưới, bộ đọc instance, `SolveResult` |
| `src/lowlevel/` | Bảng heuristic, bảng ràng buộc, Space-Time A\*, MDD |
| `src/app/` | Executable `mapf_solver`: tham số dòng lệnh, ghi JSON, đo bộ nhớ đỉnh |
| `bench/` | Đo tốc độ tầng thấp |
| `tests/` | Unit test, bộ tìm kiếm tham chiếu, test đầu-cuối, dữ liệu test |
| `third_party/doctest/` | Thư viện unit test |

Quy ước: tên file C++ dùng snake_case, mã nguồn nằm trong namespace `mapf`, và include tính từ `src/` (ví dụ `#include "core/grid.h"`).

## Môi trường đã kiểm tra

Số liệu benchmark chỉ so sánh được khi cùng compiler và cùng cờ build.

| Hệ điều hành | Compiler | CMake | Generator |
|--------------|----------|-------|-----------|
| Windows 11 | GCC 16.2.0, MSYS2 UCRT64, `x86_64-w64-mingw32` | 4.4.5 | Ninja 1.13.2 |

## Thư viện bên ngoài

| Thư viện | Phiên bản | License | Nguồn | Dùng cho |
|----------|-----------|---------|-------|----------|
| doctest | 2.5.3 | MIT | <https://github.com/doctest/doctest> | Unit test |

`third_party/doctest/doctest.h` và `LICENSE.txt` là bản sao nguyên vẹn từ tag `v2.5.3`. Lõi thuật toán do nhóm tự viết.

## Dữ liệu benchmark trong repo

`tests/data/movingai/` chứa hai file nguyên vẹn từ bộ MAPF của MovingAI (<https://movingai.com/benchmarks/mapf/index.html>), chỉ dùng cho unit test của bộ đọc instance và cho `bench_lowlevel`:

- `random-32-32-20.map`, từ `random-32-32-20.map.zip`.
- `random-32-32-20-random-1.scen`, từ `random-32-32-20.map-scen-random.zip`.

Dữ liệu cho benchmark chính không nằm trong repo.
