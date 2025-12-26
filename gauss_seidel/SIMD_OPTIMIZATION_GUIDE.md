# AVX-512 SIMD优化实现说明

## 概述

本文档说明了Gauss-Seidel求解器的AVX-512 SIMD向量化优化实现。

## 优化策略

### 1. 向量化处理
- **向量宽度**: 使用512位向量寄存器（`__m512d`），每次处理8个double值
- **批量处理**: 内循环每次处理16个点（2个AVX-512向量），减少循环开销
- **内存访问**: 使用`_mm512_loadu_pd`（非对齐加载）和`_mm512_storeu_pd`（非对齐存储）

### 2. 核心计算模式

#### 2D版本（4个邻点）
```cpp
// 加载6个向量（当前点 + 4个邻点）
__m512d u_im = _mm512_loadu_pd(&U(i-1, j));
__m512d u_ip = _mm512_loadu_pd(&U(i+1, j));
__m512d u_jm = _mm512_loadu_pd(&U(i, j-1));
__m512d u_jp = _mm512_loadu_pd(&U(i, j+1));
__m512d f_val = _mm512_loadu_pd(&F(i-1, j-1));

// 向量化计算：0.25 * (4个邻点 + h2*f)
__m512d sum = _mm512_add_pd(u_im, u_ip);
sum = _mm512_add_pd(sum, u_jm);
sum = _mm512_add_pd(sum, u_jp);
__m512d f_term = _mm512_mul_pd(vec_h2, f_val);
sum = _mm512_add_pd(sum, f_term);
__m512d result = _mm512_mul_pd(vec_025, sum);

// 存储结果
_mm512_storeu_pd(&U(i, j), result);
```

#### 3D版本（6个邻点）
```cpp
// 加载6个邻点向量
__m512d u_im = _mm512_loadu_pd(&U(i-1, j, k));
__m512d u_ip = _mm512_loadu_pd(&U(i+1, j, k));
__m512d u_jm = _mm512_loadu_pd(&U(i, j-1, k));
__m512d u_jp = _mm512_loadu_pd(&U(i, j+1, k));
__m512d u_km = _mm512_loadu_pd(&U(i, j, k-1));
__m512d u_kp = _mm512_loadu_pd(&U(i, j, k+1));
__m512d f_val = _mm512_loadu_pd(&F(i-1, j-1, k-1));

// 向量化计算：1/6 * (6个邻点 + h2*f)
__m512d sum = _mm512_add_pd(u_im, u_ip);
sum = _mm512_add_pd(sum, u_jm);
sum = _mm512_add_pd(sum, u_jp);
sum = _mm512_add_pd(sum, u_km);
sum = _mm512_add_pd(sum, u_kp);
__m512d f_term = _mm512_mul_pd(vec_h2, f_val);
sum = _mm512_add_pd(sum, f_term);
__m512d result = _mm512_mul_pd(vec_inv6, sum);

_mm512_storeu_pd(&U(i, j, k), result);
```

### 3. 循环展开策略
- **双向量展开**: 每次迭代处理2个AVX-512向量（16个点）
- **降低循环开销**: 减少循环计数器更新次数
- **提高ILP**: 两组独立计算可并行执行

### 4. 边界处理
- **标量回退**: 剩余不足16个点的使用标量代码处理
- **步长为2**: 红黑排序要求，确保处理正确的颜色点

## 性能优化要点

### 内存访问优化
1. **非对齐访问**: 
   - 使用`_mm512_loadu_pd`支持任意对齐
   - 避免内存对齐限制带来的复杂性
   - 现代CPU对非对齐访问性能损失很小

2. **访问模式**:
   - 2D: 4次邻点加载 + 1次f加载 + 1次存储 = 6次内存操作/向量
   - 3D: 6次邻点加载 + 1次f加载 + 1次存储 = 8次内存操作/向量

### 计算强度分析

#### 2D版本
- **标量版本**: 5次加载 + 5次加法 + 1次乘法 + 1次乘法 + 1次存储
- **SIMD版本**: 5次向量加载 + 5次向量加法 + 2次向量乘法 + 1次向量存储
- **理论加速比**: 
  - 数据吞吐量: 8×（向量宽度）
  - 实际受内存带宽限制

#### 3D版本
- **标量版本**: 7次加载 + 6次加法 + 2次乘法 + 1次存储
- **SIMD版本**: 7次向量加载 + 7次向量加法 + 2次向量乘法 + 1次向量存储
- **理论加速比**: 8×（若计算受限）

### 自适应分块
```cpp
int tile_size = 128;
if (N <= 64) {
    tile_size = 32;
} else if (N <= 256) {
    tile_size = 64;
}
```
- 根据问题规模调整tile大小
- 小问题使用小tile减少开销
- 大问题使用大tile提高cache利用率

## 编译选项

### 必需选项
```bash
g++ -O3 -march=native -mavx512f -fopenmp \
    gauss_seidel_2d_simd.cpp test_simd_performance_2d.cpp \
    -o test_simd_2d.exe
```

### 选项说明
- `-O3`: 启用最高级别优化
- `-march=native`: 针对当前CPU优化
- `-mavx512f`: 启用AVX-512基础指令集
- `-fopenmp`: 启用OpenMP多线程

### 可选优化选项
```bash
-mavx512dq      # AVX-512双字和四字指令
-mavx512cd      # AVX-512冲突检测指令
-mavx512vl      # AVX-512向量长度扩展
-ftree-vectorize # 自动向量化
-ffast-math     # 快速数学运算（牺牲精度）
```

## 预期性能提升

### 理论分析
1. **内存带宽优化**:
   - 减少独立内存事务数量
   - 向量加载/存储合并多个访问
   
2. **计算效率**:
   - 单指令处理多数据（8个double）
   - 减少指令数量和循环开销

3. **预期加速比**:
   - 相对标量版本: 2-4×
   - 相对tiled版本: 1.5-2.5×
   - 总体相对原始单线程: 8-16×（结合多线程）

### 实际性能因素
- **CPU微架构**: Skylake-X及更新架构支持更好
- **内存带宽**: 大问题受内存带宽限制
- **Cache层次**: 分块策略影响cache命中率
- **超线程**: 可能引入资源竞争

## 使用方法

### 单独编译运行
```bash
# 编译
g++ -O3 -march=native -mavx512f -fopenmp \
    gauss_seidel_2d.cpp \
    gauss_seidel_2d_tiled.cpp \
    gauss_seidel_2d_simd.cpp \
    test_simd_performance_2d.cpp \
    -o test_simd_2d.exe

# 运行
./test_simd_2d.exe
```

### 批量测试
```bash
# 使用PowerShell脚本
./test_simd_batch.ps1
```

### 输出格式
- **控制台**: 实时显示测试进度和结果
- **CSV文件**: `simd_results_2d.csv` 和 `simd_results_3d.csv`
  - Method: 方法名称（Original/Tiled/SIMD）
  - N: 网格尺寸
  - Threads: 线程数
  - Time(s): 运行时间
  - Iterations: 迭代次数
  - Residual: 残差
  - MaxError: 最大误差

## 技术限制

### CPU要求
- **必需**: AVX-512F基础指令集支持
- **推荐**: Intel Skylake-X或更新，AMD Zen 4或更新

### 检查CPU支持
```bash
# Windows
wmic cpu get caption

# Linux
lscpu | grep avx512
cat /proc/cpuinfo | grep avx512
```

### 已知问题
1. **频率降低**: AVX-512可能导致CPU降频
2. **内存对齐**: 虽使用非对齐指令，对齐访问性能更好
3. **边界处理**: 标量回退可能成为瓶颈

## 后续优化方向

### 进一步优化
1. **Prefetching**: 使用`_mm_prefetch`预取数据
2. **内存对齐**: 强制64字节对齐获得更好性能
3. **Mask指令**: 使用`_mm512_mask_*`完全向量化边界
4. **流式存储**: 使用`_mm512_stream_pd`减少cache污染

### 其他SIMD选项
- **AVX2**: 256位向量，更广泛的CPU支持
- **ARM NEON**: ARM处理器的SIMD指令集
- **CUDA**: GPU加速，适合超大规模问题

## 参考资料
- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [AVX-512 Programming Reference](https://www.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compiler-reference/intrinsics/intrinsics-for-avx-512-instructions.html)
- [OpenMP API Specification](https://www.openmp.org/specifications/)
