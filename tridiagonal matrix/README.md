# 三对角矩阵求解器 (TDMA Solver)

基于 OpenMP 的三对角线性方程组并行求解器，参考 [jihoonakang/parallel_tdma_cpp](https://github.com/jihoonakang/parallel_tdma_cpp)。

## 概述

实现了两种求解不可约对角占优三对角线性方程组的算法：

### 串行版本
- **Thomas 算法（追赶法）**: 经典的三对角矩阵求解算法
  - 时间复杂度: O(n)
  - 两个步骤: 前向消元 + 回代
  - 高效但无法并行化（存在数据依赖）

### 并行版本
- **PCR (Parallel Cyclic Reduction)**: 并行循环归约算法
  - 时间复杂度: O(n log n)（串行）/ O(log n)（理想并行）
  - 每次迭代消除一半方程
  - 所有更新操作在每个迭代步骤中都是独立的，适合并行计算

## 编译方法

### 手动编译
```bash
cd "tridiagonal matrix"
g++ -fopenmp -O2 -std=c++11 test_tdma.cpp tdma_solver.cpp -o test_tdma.exe
```

### 使用 Code Runner
在 VS Code 中打开 `test_tdma.cpp`，按 `Ctrl+Alt+N` 运行。

## 运行

```bash
./test_tdma.exe
```

## 文件说明

- `tdma_solver.h` - 求解器头文件
- `tdma_solver.cpp` - 求解器实现文件
- `test_tdma.cpp` - 性能测试程序

## 性能结果

程序会测试三种规模 (16384, 65536, 131072)，每种规模分别测试：
- 串行 Thomas 算法
- 并行 PCR 算法 (1, 2, 4, 8 线程)

### 典型结果

**N = 131072 (参考 tanim72/15418-final-project):**

| 方法 | 线程数 | 计算时间 | 加速比 | 并行效率 |
|------|--------|---------|--------|---------|
| Thomas | - | 1.972 ms | - | - |
| PCR | 1 | 22.074 ms | 1.00x | 100.0% |
| PCR | 2 | 23.955 ms | 0.92x | 46.1% |
| PCR | 4 | 15.395 ms | 1.43x | 35.8% |
| PCR | 8 | 12.701 ms | 1.74x | 21.7% |

**N = 65536:**

| 方法 | 线程数 | 计算时间 | 加速比 | 并行效率 |
|------|--------|---------|--------|---------|
| Thomas | - | 0.864 ms | - | - |
| PCR | 1 | 9.317 ms | 1.00x | 100.0% |
| PCR | 2 | 7.965 ms | 1.17x | 58.5% |
| PCR | 4 | 7.517 ms | 1.24x | 31.0% |
| PCR | 8 | 6.647 ms | 1.40x | 17.5% |

### 性能分析

1. **串行 Thomas vs PCR (1线程)**
   - Thomas 算法明显更快（约 10-11 倍）
   - PCR 算法的工作量更大（O(n log n) vs O(n)）
   - PCR 每次迭代需要更多内存访问和计算

2. **PCR 并行效率随规模变化**
   - **N = 16384**: 并行效率较低，线程开销大于收益
   - **N = 65536**: 开始显现并行优势，8线程达到 1.40x 加速
   - **N = 131072**: 并行效果更明显，8线程达到 1.74x 加速
   - 规模越大，并行效率越高

3. **性能瓶颈**
   - **内存带宽**: PCR 需要大量内存读写
   - **Cache 效率**: 数据访问模式对缓存不友好
   - **同步开销**: OpenMP 的 barrier 和线程调度
   - **NUMA 效应**: 多核访问内存的延迟差异

4. **何时使用 PCR？**
   - 非常大规模问题（n > 100000）
   - GPU 或众核架构（CUDA 实现性能更好）
   - 需要求解多个独立的三对角系统（批处理）
   - 分布式系统（结合 MPI）

## 算法详解

### Thomas 算法（追赶法）

求解三对角系统 Ax = d，其中:
```
A = [b₀  c₀   0  ...   0 ]
    [a₁  b₁  c₁  ...   0 ]
    [0   a₂  b₂  ...   0 ]
    [...            ... ]
    [0   ...  aₙ₋₁ bₙ₋₁]
```

**前向消元:**
```
c'₀ = c₀ / b₀
d'₀ = d₀ / b₀
for i = 1 to n-1:
    m = 1 / (bᵢ - aᵢ * c'ᵢ₋₁)
    c'ᵢ = cᵢ * m
    d'ᵢ = (dᵢ - aᵢ * d'ᵢ₋₁) * m
```

**回代:**
```
xₙ₋₁ = d'ₙ₋₁
for i = n-2 down to 0:
    xᵢ = d'ᵢ - c'ᵢ * xᵢ₊₁
```

### PCR 算法

通过递归消元，每次迭代将方程数量减半。

**归约步骤 (stride = 1, 2, 4, ...):**
对每个方程 i:
```
αᵢ = -aᵢ / bᵢ₋ₛₜᵣᵢ𝒹ₑ
γᵢ = -cᵢ / bᵢ₊ₛₜᵣᵢ𝒹ₑ

a'ᵢ = αᵢ * aᵢ₋ₛₜᵣᵢ𝒹ₑ
b'ᵢ = bᵢ + αᵢ * cᵢ₋ₛₜᵣᵢ𝒹ₑ + γᵢ * aᵢ₊ₛₜᵣᵢ𝒹ₑ
c'ᵢ = γᵢ * cᵢ₊ₛₜᵣᵢ𝒹ₑ
d'ᵢ = dᵢ + αᵢ * dᵢ₋ₛₜᵣᵢ𝒹ₑ + γᵢ * dᵢ₊ₛₜᵣᵢ𝒹ₑ
```

**最终求解:**
```
xᵢ = d'ᵢ / b'ᵢ
```

## 测试用例

生成对角占优三对角矩阵（参考 jihoonakang repo）:
```cpp
aᵢ = 1.0 + 0.01 * i
cᵢ = 1.0 + 0.02 * i
bᵢ = -(aᵢ + cᵢ) - 0.1 - 0.02 * i²
dᵢ = i
```

边界条件:
- a₀ = 0 (第一行没有下对角元素)
- cₙ₋₁ = 0 (最后一行没有上对角元素)

## 实现特点

1. **数值稳定性**: 
   - 检查除零错误
   - 使用 epsilon 阈值 (1e-15)

2. **OpenMP 并行化**:
   - `#pragma omp parallel for` 并行化归约步骤
   - 静态调度 `schedule(static)` 减少开销
   - Reduction 优化残差计算

3. **乒乓缓冲**:
   - 使用两组工作数组避免数据竞争
   - 每次迭代交替使用

4. **内存布局**:
   - 使用 `std::vector` 保证内存连续性
   - 提高 cache 命中率

## 参考文献

1. Ji-Hoon Kang, [parallel_tdma_cpp](https://github.com/jihoonakang/parallel_tdma_cpp), 2019
2. tanim72, [15418-final-project](https://github.com/tanim72/15418-final-project) - OpenMP/MPI/CUDA implementations
3. Karniadakis & Kirby, *Parallel Scientific Computing in C++ and MPI*, Cambridge University Press
4. Laszlo, Gilles & Appleyard, "Manycore Algorithms for Batch Scalar and Block Tridiagonal Solvers", *ACM TOMS*, 42, 31 (2016)

## 说明

本实现主要用于**教学和算法演示**目的，测试规模参考 tanim72/15418-final-project (N = 131072)。

对于实际工程应用：
- 小规模问题（N < 10000）推荐使用串行 Thomas 算法
- 中等规模问题（10000 < N < 100000）考虑优化的串行实现
- 大规模问题（N > 100000）可考虑：
  - GPU 加速的 PCR/CR 算法（CUDA 实现）
  - MPI 分布式并行（Recursive Doubling, Brugnano 算法）
  - 混合 OpenMP + MPI
- 生产环境建议使用成熟的线性代数库（如 Intel MKL, LAPACK, cuSPARSE 等）
