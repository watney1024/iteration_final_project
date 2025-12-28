# 并行计算期末项目

本项目为迭代方法与预处理课程期末大作业，主要研究 OpenMP 在不同科学计算和神经网络算子中的并行优化技术及性能分析。

## 项目概述

项目包含三个主要模块，分别实现了不同计算场景下的并行优化方案：

1. **神经网络算子**（卷积与平均池化的并行实现）
2. **Gauss-Seidel 迭代法**（2D/3D 泊松方程求解）
3. **三对角矩阵求解器**（多种并行算法对比）


每个模块都包含多种优化版本，通过对比实验分析不同并行策略和优化技术的性能表现。

---

## 环境要求

- **操作系统**: Windows
- **编译器**: g++ (MinGW-w64 或 MSYS2)
- **并行库**: OpenMP（g++ 自带，使用 `-fopenmp` 编译选项）
- **脚本运行**: PowerShell 5.0+

### 编译命令示例

```bash
# 基本编译命令（串行）
g++ -O2 -std=c++11 -o program.exe source.cpp

# 启用 OpenMP 并行
g++ -fopenmp -O2 -std=c++11 -o program.exe source.cpp

# 启用本地架构优化
g++ -fopenmp -O2 -march=native -std=c++11 -o program.exe source.cpp
```

---

## 模块详解
### 1. 神经网络算子 (`operators/`)

实现深度学习中常用的卷积和池化算子：

**算子实现**:

**卷积算子 (Convolution)**:
- `conv.cpp`: 串行实现（基准）
- `conv_openmp.cpp`: OpenMP 并行实现
- `conv_openmp_optimized.cpp`: 优化版本（循环重排、缓存优化）

**平均池化算子 (Average Pooling)**:
- `avgpool.cpp`: 串行实现（基准）
- `avgpool_openmp.cpp`: OpenMP 并行实现
- `avgpool_openmp_memory.cpp`: 内存优化版本（mallopt、2×2 特化、指针优化）

**运行测试**:
```powershell
cd operators

# 测试卷积算子性能
.\test_conv_threads.ps1

# 测试池化算子性能（多线程）
.\test_avgpool_threads.ps1

# 测试池化算子内存优化版本
.\test_avgpool_memory.ps1
```

**测试内容**:
- 不同线程数（1, 2, 4, 8, 10, 16, 20）
- 固定输入规模测试（卷积: 3×150×150，池化: 320×300×300）
- 统计中位数时间和 P99 延迟

**结果输出**:
- `conv_openmp_results.txt`: 卷积算子测试结果
- `avgpool_openmp_results.txt`: 池化算子测试结果


### 2. Gauss-Seidel 迭代法 (`gauss_seidel/`)

实现二维和三维泊松方程的迭代求解器，包含多种优化版本：

**文件结构**:
- `gauss_seidel_2d.cpp/h`: 2D 求解器（串行、红黑着色、OpenMP 并行）
- `gauss_seidel_2d_tiled.cpp`: 2D 分块优化版本
- `gauss_seidel_2d_tiled_aligned.cpp`: 2D 分块 + 内存对齐优化
- `gauss_seidel_3d.cpp/h`: 3D 求解器（串行、红黑着色、OpenMP 并行）
- `gauss_seidel_3d_tiled.cpp`: 3D 分块优化版本
- `gauss_seidel_3d_tiled_aligned.cpp`: 3D 分块 + 内存对齐优化
- `test_all.ps1`: 批量测试脚本

**运行测试**:
```powershell
cd gauss_seidel
.\test_all.ps1
```

**测试内容**:
- 不同网格规模（64×64 到 2048×2048 或 64³ 到 512³）
- 不同线程数（1, 2, 4, 8, 10, 16, 20）
- 对比原始版本、分块优化、内存对齐优化的性能

**结果输出**:
- `aligned_results_<timestamp>/results_2d.csv`: 2D 测试结果
- `aligned_results_<timestamp>/results_3d.csv`: 3D 测试结果

---

### 3. 三对角矩阵求解器 (`new_tri/`)

实现多种三对角线性方程组求解算法：

**算法实现**:
- `sequential_solver_memtest.cpp`: 串行 Thomas 算法（基准）
- `openmp_brugnano_memtest.cpp`: Brugnano 并行算法
- `openmp_recursive_doubling_memtest.cpp`: Recursive Doubling 并行算法

**运行测试**:
```powershell
cd new_tri
.\batch_test_memtest.ps1
```

**测试内容**:
- 不同数据规模（8k, 16k, 128k, 1M, 4M, 8M, 16M, 32M）
- 不同线程数（1, 2, 4, 8, 16, 20）
- 对比串行与两种并行算法的性能

**结果输出**:
- `batch_results_memtest_<timestamp>/<size>_results.csv`: 各规模测试结果
- `batch_results_memtest_<timestamp>/summary_all.csv`: 汇总结果

---



## 优化技术总结

本项目应用了以下并行计算和性能优化技术：

### 并行化策略
-  **OpenMP 多线程并行**：fork-join 模型，数据级并行
-  **红黑着色法**：消除 Gauss-Seidel 迭代的数据依赖
-  **循环并行化**：`#pragma omp parallel for` 自动任务分配

### 算法优化
-  **Brugnano 并行算法**：三对角矩阵并行求解
-  **Recursive Doubling**：对数级并行深度的三对角求解
-  **循环重排**：提高缓存命中率

### 内存优化
-  **分块优化 (Tiling)**：提高数据局部性，减少缓存缺失
-  **内存对齐 (Alignment)**：利用 SIMD 指令，提高访问效率
-  **指针优化**：预计算地址偏移，减少索引计算开销
-  **特化优化**：针对 2×2 池化核的展开优化

---

## 性能分析

### 结果文件说明

测试脚本会生成 CSV 格式的结果文件，包含以下关键指标：

- **执行时间 (Time)**: 以毫秒或秒为单位
- **加速比 (Speedup)**: 相对于串行版本或单线程的性能提升
- **线程数 (Threads)**: 使用的并行线程数
- **数据规模 (Size)**: 问题规模（网格大小、矩阵维度等）

### 数据分析建议

1. **加速比曲线**: 绘制线程数 vs 加速比，分析并行效率
2. **强扩展性**: 固定问题规模，增加线程数
3. **弱扩展性**: 问题规模与线程数同步增长
4. **优化对比**: 对比不同优化技术的性能提升

---

## 项目结构

```
iteration_final_project/
├── gauss_seidel/              # Gauss-Seidel 迭代求解器
│   ├── gauss_seidel_2d*       # 2D 实现（原始/分块/对齐）
│   ├── gauss_seidel_3d*       # 3D 实现（原始/分块/对齐）
│   ├── test_all.ps1           # 批量测试脚本
│   └── aligned_results_*/     # 测试结果目录
│
├── new_tri/                   # 三对角矩阵求解器
│   ├── sequential_solver_memtest.cpp      # 串行算法
│   ├── openmp_brugnano_memtest.cpp        # Brugnano 并行
│   ├── openmp_recursive_doubling_memtest.cpp  # Recursive Doubling
│   ├── batch_test_memtest.ps1             # 批量测试脚本
│   └── batch_results_memtest_*/           # 测试结果目录
│
├── operators/                 # 神经网络算子
│   ├── conv*.cpp              # 卷积算子实现
│   ├── avgpool*.cpp           # 池化算子实现
│   ├── test_*.ps1             # 各算子测试脚本
│   └── *_results.txt          # 测试结果文件
│
├── latex/                     # 论文 LaTeX 源码
│   ├── main.tex               # 主文档
│   └── figures/               # 图表目录
│
├── README.md                  # 本文件
└── performance.txt            # 性能数据摘要
```

---

## 快速开始

### 全部测试流程

```powershell
# 1. 测试 Gauss-Seidel 求解器
cd gauss_seidel
.\test_all.ps1
cd ..

# 2. 测试三对角矩阵求解器
cd new_tri
.\batch_test_memtest.ps1
cd ..

# 3. 测试神经网络算子
cd operators
.\test_conv_threads.ps1
.\test_avgpool_threads.ps1
.\test_avgpool_memory.ps1
cd ..
```

### 单独编译示例

```powershell
# 编译 2D Gauss-Seidel 测试
cd gauss_seidel
g++ -O2 -march=native -fopenmp `
    gauss_seidel_2d.cpp `
    gauss_seidel_2d_tiled.cpp `
    gauss_seidel_2d_tiled_aligned.cpp `
    test_tiled_aligned_2d.cpp `
    -o test_aligned_2d.exe

# 编译三对角求解器
cd new_tri
g++ -fopenmp -O2 -std=c++11 -o openmp_brugnano_memtest.exe openmp_brugnano_memtest.cpp

# 编译卷积算子
cd operators
g++ -fopenmp -O2 -std=c++11 -o conv_openmp.exe conv_openmp.cpp
```


## 参考资料

### 官方文档
- [OpenMP 官方文档](https://www.openmp.org/specifications/)

### 相关开源项目
- [EasyNN - 神经网络算子实现](https://github.com/HuPengsheet/EasyNN)
- [线性方程组并行求解](https://github.com/maitreyeepaliwal/Solving-System-of-linear-equations-in-parallel-and-serial.git)
- [Gauss-Seidel 迭代优化](https://github.com/gulang2019/optimizing-gauss-seidel-iteration.git)
- [并行 TDMA 算法 C++ 实现](https://github.com/jihoonakang/parallel_tdma_cpp.git)
- [并行计算期末项目参考](https://github.com/tanim72/15418-final-project.git)

