# Đặc tả mô hình MAPF

Tài liệu này cố định mô hình Multi-Agent Path Finding mà mọi thành phần trong repo dùng chung: tìm kiếm tầng thấp, CBS, ECBS, phát hiện xung đột, validator và oracle. Khi code và tài liệu này khác nhau thì tài liệu này đúng, và code phải được sửa.

Mô hình theo định nghĩa MAPF cổ điển của Stern và cộng sự (SoCS 2019, arXiv:1906.08291) và bài báo CBS của Sharon và cộng sự (AIJ 2015).

Trạng thái: bản nháp của TV1, chờ TV2, TV3 và TV4 review ở Gate W1.

## 1. Bản đồ

- Bản đồ là một lưới chữ nhật `height × width`. Mỗi ô hoặc đi được, hoặc là vật cản.
- Một ô được định danh bằng số nguyên `cell = row * width + col`. Hàng 0 ở trên cùng, cột 0 ở bên trái.
- Lưới 4 hướng: hai ô kề nhau khi chúng chung một cạnh. Không có nước đi chéo.
- Thứ tự liệt kê ô kề luôn là lên, xuống, trái, phải. Thứ tự cố định này làm mọi phép tìm kiếm cho kết quả xác định.

## 2. Agent, thời gian và hành động

- Có `k` agent. Agent `a_i` có ô xuất phát `s_i` và ô đích `g_i`, đều là ô đi được.
- Hai agent khác nhau có ô xuất phát khác nhau và ô đích khác nhau. Ô đích của một agent được phép trùng ô xuất phát của agent khác.
- Thời gian rời rạc: `t = 0, 1, 2, ...`. Tại `t = 0` mọi agent ở ô xuất phát.
- Mỗi bước thời gian, một agent hoặc đi sang một ô kề đi được, hoặc đứng yên. Cả hai hành động đều tốn 1.

## 3. Đường đi và stay-at-target

Đường đi của agent `a_i` là dãy ô `π_i[0], π_i[1], ..., π_i[T_i]`, trong đó `π_i[t]` là ô agent chiếm tại thời điểm `t`.

- `π_i[0] = s_i` và `π_i[T_i] = g_i`.
- Hai phần tử liên tiếp hoặc bằng nhau (đứng yên), hoặc là hai ô kề nhau.
- **Stay-at-target:** sau `T_i` agent đứng yên tại đích mãi mãi. Với mọi `t > T_i`, quy ước `π_i(t) = g_i`.
- **Dạng chuẩn:** `T_i` là thời điểm nhỏ nhất mà từ đó agent ở đích và không rời đi nữa. Vì vậy nếu đường đi có hơn một phần tử thì phần tử kế cuối khác `g_i`. Trong code, `Path` luôn ở dạng chuẩn và `T_i = path.size() - 1`.

Agent được phép đi qua đích của chính nó, rời đi rồi quay lại. Khi đó `T_i` là lần đến cuối cùng.

```
Đường đi:  s   x   g   y   g        T_i = 4, không phải 2
Thời điểm: 0   1   2   3   4
```

## 4. Xung đột

Hai đường đi được so sánh trên mọi thời điểm `t ≥ 0`. Đường ngắn hơn được kéo dài bằng ô đích theo quy ước stay-at-target.

### 4.1. Vertex conflict

`(a_i, a_j, v, t)`: hai agent cùng ở ô `v` tại thời điểm `t`, tức `π_i(t) = π_j(t) = v`.

```
Thời điểm:  0    1
a_i:        A -> B
a_j:        C -> B        cả hai ở B tại t = 1
```

### 4.2. Edge conflict

`(a_i, a_j, u, v, t)`: hai agent đổi chỗ cho nhau qua cùng một cạnh. Agent `a_i` đi `u → v` và agent `a_j` đi `v → u`, cả hai đến nơi tại thời điểm `t`:

`π_i(t-1) = u`, `π_i(t) = v`, `π_j(t-1) = v`, `π_j(t) = u`.

```
Thời điểm:  t-1    t
a_i:         u  ->  v
a_j:         v  ->  u     đổi chỗ trên cạnh (u, v)
```

Mọi thời điểm gắn với một cạnh đều là **thời điểm đến**.

### 4.3. Xung đột tại đích

Agent đã dừng tại đích vẫn chiếm ô đó. Nếu `a_i` đến đích tại `T_i` và `a_j` đi vào `g_i` tại thời điểm `t ≥ T_i`, đó là vertex conflict `(a_i, a_j, g_i, t)` như mọi vertex conflict khác.

```
Thời điểm:  0    1    2    3    4    5
a_i:        .    .    g    g    g    g      T_i = 2, sau đó đứng yên tại g
a_j:        .    .    .    .    .    g      vertex conflict (a_i, a_j, g, 5)
```

### 4.4. Không phải xung đột

Đi nối đuôi không phải xung đột: một agent được vào ô `v` tại thời điểm `t` khi agent khác rời `v` cũng tại thời điểm `t`, miễn là hai agent không đổi chỗ cho nhau.

```
Thời điểm:  t-1    t
a_i:         u  ->  v
a_j:         v  ->  w     hợp lệ: a_j rời v đúng lúc a_i vào v
```

## 5. Lời giải và hàm mục tiêu

- Một **lời giải** là `k` đường đi, mỗi agent một đường. Lời giải **hợp lệ** khi mọi đường đi thoả mục 3 và không cặp đường nào có vertex conflict hay edge conflict.
- Hàm mục tiêu chính là **sum-of-costs**: `SOC = Σ T_i`.
  - Các bước đứng yên trước lần đến đích cuối cùng có tính vào chi phí.
  - Đứng yên tại đích sau `T_i` không tính.
- `SOC*` là SOC nhỏ nhất trên mọi lời giải hợp lệ.
- Metric phụ: `makespan = max T_i`.

Ví dụ: agent đứng yên một bước rồi đi hai bước tới đích có `T_i = 3`. Agent khác đến đích tại `t = 2` rồi đứng đó có `T_j = 2`. Khi đó `SOC = 5` và `makespan = 3`.

## 6. Ràng buộc

CBS và ECBS giải xung đột bằng cách áp ràng buộc lên từng agent.

- **Ràng buộc vertex** `(a, v, t)`: agent `a` không được ở ô `v` tại thời điểm `t`.
- **Ràng buộc edge** `(a, u, v, t)`: agent `a` không được đi `u → v` để đến `v` tại thời điểm `t`. Nó cấm cặp `π_a(t-1) = u`, `π_a(t) = v`. Ràng buộc này có hướng: nó không cấm đi `v → u`.

Cách tách một xung đột thành hai ràng buộc:

| Xung đột | Ràng buộc cho `a_i` | Ràng buộc cho `a_j` |
|----------|---------------------|---------------------|
| Vertex `(a_i, a_j, v, t)` | `(a_i, v, t)` | `(a_j, v, t)` |
| Edge `(a_i, a_j, u, v, t)` | `(a_i, u, v, t)` | `(a_j, v, u, t)` |

Hệ quả của stay-at-target: nếu agent `a` có ràng buộc vertex `(a, g_a, t)` tại đích của chính nó, thì agent chưa được dừng tại đích ở bất kỳ thời điểm nào không lớn hơn `t`. Đường đi hợp lệ của agent phải có `T_a ≥ t + 1`.

## 7. Kiểu dữ liệu trong code

Các kiểu sau nằm trong [`src/core/types.h`](../src/core/types.h) và mang đúng ngữ nghĩa của tài liệu này.

- `Path`: `std::vector<int>`, chỉ số là thời điểm, giá trị là ô; luôn ở dạng chuẩn của mục 3.
- `Constraint`: gồm `agent`, `type` (`Vertex` hoặc `Edge`), `from`, `to`, `timestep`. Với ràng buộc vertex, `to` là ô bị cấm và `from = -1`. Với ràng buộc edge, agent bị cấm đi `from → to` và đến tại `timestep`.
- `SearchResult`: kết quả một lần tìm đường cho một agent. `cost` là `T_i` của đường trả về. `lower_bound` là giá trị f nhỏ nhất còn trong OPEN lúc dừng. Tìm kiếm tối ưu trả `lower_bound == cost`; tìm kiếm bounded-suboptimal có thể trả `lower_bound < cost`. Hai trường này không bao giờ được gộp làm một.

## 8. Định dạng MovingAI

Benchmark dùng bộ dữ liệu MAPF của MovingAI (<https://movingai.com/benchmarks/mapf/index.html>). Code đọc trực tiếp hai loại file.

**File `.map`**

```
type octile
height 4
width 5
map
.....
.@@..
..T..
....@
```

- Sau dòng `map` là đúng `height` dòng, mỗi dòng đúng `width` ký tự.
- Ô đi được: `.`, `G`, `S`. Vật cản: `@`, `O`, `T`, `W`. Ký tự khác là lỗi.
- Dòng `type octile` được bỏ qua: mô hình của chúng ta luôn là lưới 4 hướng.

**File `.scen`**

```
version 1
0	tiny.map	5	4	0	0	4	0	4.00000000
```

- Dòng đầu là `version`. Mỗi dòng sau là một agent, gồm 9 trường cách nhau bằng tab: bucket, tên map, width, height, start x, start y, goal x, goal y, độ dài tối ưu.
- `x` là cột và `y` là hàng, gốc `(0, 0)` ở góc trên trái. Ô tương ứng là `y * width + x`.
- Instance mức `k` agent lấy `k` dòng agent đầu tiên của file.
- Trường độ dài tối ưu là khoảng cách trên lưới 8 hướng của MovingAI. Code không dùng trường này.

Loader từ chối file khi: thiếu header, số dòng hoặc số ký tự không khớp, kích thước trong `.scen` khác bản đồ, toạ độ nằm ngoài bản đồ hoặc trên vật cản, hai agent trùng ô xuất phát hoặc trùng ô đích, hoặc `k` lớn hơn số dòng agent.
