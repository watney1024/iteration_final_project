# Gauss-Seidel 测试脚本使用指南

## 概述

本目录包含了2D和3D泊松方程Gauss-Seidel求解器的批量测试脚本。

## 文件说明

### 源代码
- `gauss_seidel_2d.cpp/h` - 2D求解器实现
- `gauss_seidel_3d.cpp/h` - 3D求解器实现
- `test_gs_2d_batch.cpp` - 2D批量测试程序（支持命令行参数）
- `test_gs_3d_batch.cpp` - 3D批量测试程序（支持命令行参数）

### 测试脚本

#### 批量测试脚本（完整测试所有尺寸）
- `test_gs_2d_batch.ps1` - 2D批量测试脚本
  - 测试尺寸：128, 256, 512, 1024
  - 线程数：1, 2, 4, 8, 10, 16, 20
  - 结果保存在 `results_2d/` 目录

- `test_gs_3d_batch.ps1` - 3D批量测试脚本
  - 测试尺寸：128³, 256³, 512³
  - 线程数：1, 2, 4, 8, 10, 16, 20
  - 结果保存在 `results_3d/` 目录

#### 快速测试脚本（单一尺寸）
- `quick_test_2d.ps1` - 2D快速测试
  - 默认尺寸：512×512
  - 可自定义尺寸和线程数
  
- `quick_test_3d.ps1` - 3D快速测试
  - 默认尺寸：256³
  - 可自定义尺寸和线程数

## 使用方法

### 1. 批量测试（推荐用于完整性能分析）

#### 2D批量测试
```powershell
# 运行所有尺寸的测试
powershell -ExecutionPolicy ByPass -File test_gs_2d_batch.ps1
```

#### 3D批量测试
```powershell
# 运行所有尺寸的测试（注意：512³需要约8GB内存和较长时间）
powershell -ExecutionPolicy ByPass -File test_gs_3d_batch.ps1
```

### 2. 快速测试（推荐用于验证和调试）

#### 2D快速测试
```powershell
# 使用默认设置（512×512）
powershell -ExecutionPolicy ByPass -File quick_test_2d.ps1

# 自定义网格尺寸
powershell -ExecutionPolicy ByPass -File quick_test_2d.ps1 -GridSize 1024

# 自定义线程数
powershell -ExecutionPolicy ByPass -File quick_test_2d.ps1 -GridSize 512 -ThreadCounts @(1,4,8,16)
```

#### 3D快速测试
```powershell
# 使用默认设置（256³）
powershell -ExecutionPolicy ByPass -File quick_test_3d.ps1

# 自定义网格尺寸（小心内存使用）
powershell -ExecutionPolicy ByPass -File quick_test_3d.ps1 -GridSize 128

# 自定义线程数
powershell -ExecutionPolicy ByPass -File quick_test_3d.ps1 -GridSize 256 -ThreadCounts @(1,4,8)
```

### 3. 单次测试（手动运行）

```powershell
# 编译
g++ -O2 -std=c++11 -fopenmp -o test_gs_2d_batch.exe test_gs_2d_batch.cpp gauss_seidel_2d.cpp

# 运行（格式：程序名 网格尺寸 线程数）
.\test_gs_2d_batch.exe 512 8
```

## 输出说明

### 批量测试输出
- 完整日志：`results_2d/gs2d_<尺寸>_t<线程数>_<时间戳>.txt`
- 汇总结果：`results_2d/summary_2d_<时间戳>.txt`

### 快速测试输出
- 直接输出到控制台
- 包含时间、加速比、并行效率

### 关键指标
- **迭代次数**：达到收敛所需的迭代次数
- **总计算时间**：求解器运行时间（不含初始化）
- **每次迭代时间**：平均每次迭代耗时
- **加速比**：相对于单线程的性能提升
- **并行效率**：加速比/线程数（理想值100%）
- **GFLOPS**（仅3D）：浮点运算性能

## 内存需求

### 2D问题
- 128×128：约 0.1 MB
- 256×256：约 0.5 MB
- 512×512：约 2 MB
- 1024×1024：约 8 MB

### 3D问题
- 128³：约 48 MB
- 256³：约 384 MB
- 512³：约 3 GB

**注意**：512³测试需要至少4GB可用内存，建议8GB以上。

## 性能优化特性

1. **红黑排序**：消除数据依赖，允许并行化
2. **分块策略**：提高缓存局部性（2D: 32×32, 3D: 64³）
3. **OpenMP并行**：多核并行加速
4. **静态调度**：减少线程调度开销
5. **向量化**：编译器自动SIMD优化

## 预期性能

### 2D (512×512)
- 串行：~100 ms
- 8线程：~20 ms (5×加速)
- 16线程：~12 ms (8×加速)

### 3D (256³)
- 串行：~10 s
- 8线程：~2 s (5×加速)
- 16线程：~1.2 s (8×加速)

### 3D (512³)
- 串行：~180 s
- 8线程：~30 s
- 16线程：~18 s

**注意**：实际性能取决于CPU型号、内存带宽和系统负载。

## 故障排除

### 问题1：编译失败
**原因**：缺少OpenMP支持
**解决**：确保使用支持OpenMP的编译器（GCC 4.2+, MSVC 2015+）

### 问题2：内存不足
**原因**：3D大规模问题需要大量内存
**解决**：减小网格尺寸，或关闭其他应用程序

### 问题3：性能不佳
**原因**：系统负载高、超线程影响、NUMA效应
**解决**：
- 关闭后台应用
- 尝试不同线程数（避免超线程，如10核20线程的CPU用10线程）
- 检查CPU温度和频率

### 问题4：未收敛
**原因**：迭代次数不足
**解决**：修改源代码中的 `max_iter` 参数

## 算法参考

本实现参考了以下论文：
- Adams et al. "Parallel multigrid smoothing: polynomial versus Gauss-Seidel"
- Zhang et al. "A comparison of domain decomposition and red-black ordering"

## 许可证

本代码用于学术研究和教学目的。
