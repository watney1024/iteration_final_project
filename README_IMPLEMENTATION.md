# 红黑排序 Gauss-Seidel 并行算法实现

这是一个基于区域分解的红黑排序 Gauss-Seidel 算法的 C++ 实现，使用 OpenMP 进行并行计算。

## 算法说明

### 红黑排序 Gauss-Seidel 方法

红黑排序（Red-Black Ordering）是一种特殊的迭代方法，它将求解域的所有点分为两类：
- **红点**：索引和为偶数的点 `(i+j) % 2 == 0`
- **黑点**：索引和为奇数的点 `(i+j) % 2 == 1`

### 算法步骤

1. **初始化**：设置初始解向量 x = 0
2. **红点更新**：并行更新所有红点（红点之间相互独立）
   ```
   x_i^(k+1) = (b_i - Σ(A_ij * x_j)) / A_ii
   ```
3. **黑点更新**：并行更新所有黑点（黑点之间相互独立）
4. **收敛检查**：计算残差，若满足收敛条件则停止
5. **迭代**：重复步骤 2-4 直到收敛或达到最大迭代次数

### 并行化策略

- 使用 OpenMP 的 `#pragma omp parallel for` 指令
- 红点和黑点分别进行并行更新
- 使用 `#pragma omp barrier` 确保同步
- 采用静态调度策略 `schedule(static)` 以提高缓存命中率

## 文件说明

- `red_black_gauss_seidel.h` - 头文件，定义求解器类接口
- `red_black_gauss_seidel.cpp` - 实现文件，包含串行和并行算法
- `test_performance.cpp` - 性能测试程序，测试不同线程数下的加速比
- `test_correctness.cpp` - 正确性验证程序
- `Makefile` - 编译脚本

## 编译方法

### Windows (MinGW/MSYS2)

确保已安装支持 OpenMP 的 g++ 编译器：

```bash
# 编译性能测试程序
g++ -fopenmp -O3 -std=c++11 test_performance.cpp red_black_gauss_seidel.cpp -o test_performance.exe

# 编译正确性测试程序
g++ -fopenmp -O3 -std=c++11 test_correctness.cpp red_black_gauss_seidel.cpp -o test_correctness.exe

# 或使用 Makefile
make
```

### Linux

```bash
# 编译性能测试程序
g++ -fopenmp -O3 -std=c++11 test_performance.cpp red_black_gauss_seidel.cpp -o test_performance

# 编译正确性测试程序
g++ -fopenmp -O3 -std=c++11 test_correctness.cpp red_black_gauss_seidel.cpp -o test_correctness

# 或使用 Makefile
make
```

## 运行方法

### 正确性测试

```bash
# Windows
.\test_correctness.exe

# Linux
./test_correctness
```

### 性能测试

```bash
# Windows
.\test_performance.exe

# Linux
./test_performance
```

性能测试将自动运行以下配置：
- 线程数：1, 2, 4, 8（根据系统可用线程数自动调整）
- 矩阵规模：100×100, 500×500, 1000×1000
- 最大迭代次数：1000
- 收敛容差：1e-6

## 性能结果示例

测试输出将包含：
- 每个配置的执行时间
- 相对于串行版本的加速比
- 最终残差（验证收敛性）

示例输出：
```
========================================
Performance Test
Matrix size: 500x500
Max iterations: 1000
Tolerance: 1e-06
========================================

Serial Version:
  Time: 2.456789 seconds
  Final residual: 9.8765e-07

Parallel Version Results:
   Threads        Time (s)        Speedup            Residual
------------------------------------------------------------
         1        2.458123          0.999        9.8765e-07
         2        1.289456          1.905        9.8765e-07
         4        0.687234          3.574        9.8765e-07
         8        0.423567          5.801        9.8765e-07
```

## 理论分析

### 时间复杂度
- 单次迭代：O(n²)，其中 n 是矩阵维度
- 总时间：O(k·n²)，其中 k 是迭代次数

### 空间复杂度
- O(n²) 用于存储矩阵 A
- O(n) 用于存储向量 b 和 x

### 加速比分析
理论加速比受以下因素影响：
1. **Amdahl 定律**：串行部分限制了最大加速比
2. **缓存效应**：数据局部性影响性能
3. **负载均衡**：红点和黑点数量的平衡
4. **同步开销**：线程同步的时间成本

## 参考资料

1. [Solving System of linear equations in parallel and serial](https://github.com/maitreyeepaliwal/Solving-System-of-linear-equations-in-parallel-and-serial.git)
2. [Optimizing Gauss-Seidel Iteration](https://github.com/gulang2019/optimizing-gauss-seidel-iteration.git)

## 注意事项

1. **对角占优**：算法要求矩阵严格对角占优以保证收敛
2. **线程数设置**：建议不超过系统物理核心数
3. **矩阵规模**：大规模矩阵能获得更好的并行效率
4. **编译优化**：使用 `-O3` 优化标志以获得最佳性能

## 扩展方向

1. 实现基于区域分解的版本（Domain Decomposition）
2. 添加 MPI 支持以实现分布式并行
3. 优化数据结构（稀疏矩阵存储）
4. 实现自适应线程数选择
5. 添加预条件子以加速收敛
