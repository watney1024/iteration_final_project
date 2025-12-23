# 二维泊松方程 Gauss-Seidel 求解器

基于区域分解的红黑排序 Gauss-Seidel 算法实现，使用 C++ 和 OpenMP 进行多线程并行化。

## 项目结构

```
├── gauss_seidel_2d.h          # 头文件，定义接口
├── gauss_seidel_2d.cpp        # 实现文件
├── test_gs_2d.cpp             # 测试和性能对比程序
└── README.md                  # 本文件
```

## 实现的算法

### 1. 串行普通 Gauss-Seidel
标准的 Gauss-Seidel 迭代方法，按行扫描顺序更新所有网格点。

### 2. 串行红黑 Gauss-Seidel
使用红黑排序的 Gauss-Seidel 方法：
- **红点**: (i+j) % 2 == 0
- **黑点**: (i+j) % 2 == 1
- 先更新所有红点，再更新所有黑点

### 3. 并行红黑 Gauss-Seidel（OpenMP + 区域分解）
基于区域分解和多级分块的并行实现：

#### 核心技术
- **红黑排序**: 消除数据依赖，使得同颜色的点可以并行更新
- **区域分解**: 将问题域划分为多个子区域
- **多级分块**: 
  - 第一级：按行将区域分配给不同线程
  - 第二级：每个区域内使用 8×8 小块，提高缓存局部性
- **OpenMP 并行**: 使用静态调度减少线程同步开销

#### 并行策略
```
红点更新阶段：
  #pragma omp parallel for
  for each tile:
    更新tile内所有红点（无数据依赖，可并行）

黑点更新阶段：
  #pragma omp parallel for
  for each tile:
    更新tile内所有黑点（无数据依赖，可并行）
```

## 编译和运行

### 编译
```bash
g++ -fopenmp -O2 -std=c++11 test_gs_2d.cpp gauss_seidel_2d.cpp -o test_gs_2d.exe
```

### 运行
```bash
./test_gs_2d.exe
```

或在 Windows PowerShell:
```powershell
.\test_gs_2d.exe
```

## 测试问题

求解二维泊松方程：
```
-Δu = f,  在 Ω = (0,1) × (0,1)
u = 0,    在 ∂Ω
```

使用制造解验证：
- **精确解**: u(x,y) = sin(πx) · sin(πy)
- **右端项**: f(x,y) = 2π² · sin(πx) · sin(πy)

## 性能指标

程序输出以下性能指标：
- **迭代次数**: 达到收敛所需的迭代次数
- **最终残差**: L2 范数残差
- **相对误差**: 相对于精确解的相对误差
- **计算时间**: 总计算时间（毫秒）
- **每次迭代时间**: 平均每次迭代的时间
- **加速比**: 并行版本相对于串行版本的速度提升

## 关键实现细节

### 红黑排序
```cpp
// 红点: (i+j) % 2 == 0
for (int i = 1; i <= N; ++i) {
    for (int j = 1; j <= N; ++j) {
        if ((i + j) % 2 == 0) {
            // 更新 u(i,j)
        }
    }
}

// 黑点: (i+j) % 2 == 1
for (int i = 1; i <= N; ++i) {
    for (int j = 1; j <= N; ++j) {
        if ((i + j) % 2 == 1) {
            // 更新 u(i,j)
        }
    }
}
```

### 分块优化
```cpp
const int tile_size = 8;  // 8×8 小块

#pragma omp parallel for schedule(static)
for (int block_i = 1; block_i <= N; block_i += tile_size) {
    for (int block_j = 1; block_j <= N; block_j += tile_size) {
        // 在块内更新
        for (int i = block_i; i < min(block_i + tile_size, N+1); ++i) {
            for (int j = block_j; j < min(block_j + tile_size, N+1); ++j) {
                // 更新 u(i,j)
            }
        }
    }
}
```

## 参考资料

本实现参考了以下论文和项目：
- [Optimizing Gauss-Seidel Iteration](https://github.com/gulang2019/optimizing-gauss-seidel-iteration)
- Red-Black Gauss-Seidel 用于并行化迭代方法
- 区域分解方法在偏微分方程求解中的应用

## 预期结果

对于 64×64 网格：
- **串行普通 GS**: 基准性能
- **串行红黑 GS**: 收敛速度略好于普通 GS
- **并行红黑 GS**: 
  - 2 线程：加速比 ~1.5x
  - 4 线程：加速比 ~2.5x
  - 8 线程：加速比 ~3.5x（受限于内存带宽和同步开销）

## 技术要点

1. **数据局部性**: 8×8 分块优化了 L1 缓存利用率
2. **负载均衡**: 静态调度确保线程负载均衡
3. **同步开销**: 使用 `nowait` 子句减少不必要的同步
4. **可扩展性**: 支持不同线程数和网格规模

---

## 原始课程要求

latex排版，首页写上标题、学号、姓名，不少于 6页 A4纸。

使用openmp并行计算：

写出算法，并对算法进行测试，列出不同线程（nthreads=1,2,4,8）时所需的计算时间，计算加速比。

+ 基于区域分解的红黑排序 Gauss-Seidel 算法及其实现
参考repo：
1. [https://github.com/maitreyeepaliwal/Solving-System-of-linear-equations-in-parallel-and-serial.git](https://github.com/maitreyeepaliwal/Solving-System-of-linear-equations-in-parallel-and-serial.git)

2. [https://github.com/gulang2019/optimizing-gauss-seidel-iteration.git](https://github.com/gulang2019/optimizing-gauss-seidel-iteration.git)

+ 求解不可约对角占优的三对角线性方程组的追赶法的并行算法及其实现

1. [https://github.com/xccels/PaScaL_TDMA.git](https://github.com/xccels/PaScaL_TDMA.git)

2. [https://github.com/jihoonakang/parallel_tdma_cpp.git](https://github.com/jihoonakang/parallel_tdma_cpp.git)

3. [https://github.com/tanim72/15418-final-project.git](https://github.com/tanim72/15418-final-project.git)







# 2025年12月23日

初步完成了代码的创建，主要为3部分

1. 之前的神经网络算子
2. gauss seidel
3. Thomas算法

接下来需要做的事：

1. 确定合适的数据集规模
2. 搞清楚代码结构
3. 多跑实验
4. 看看有没有其他优化方法，比如share memory等操作，进行消融实验