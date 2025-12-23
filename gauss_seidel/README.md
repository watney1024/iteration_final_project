# Gauss-Seidel 求解器

基于区域分解的红黑排序 Gauss-Seidel 算法实现（参考 [gulang2019/optimizing-gauss-seidel-iteration](https://github.com/gulang2019/optimizing-gauss-seidel-iteration.git)）。

## 实现版本

### 二维版本 (64×64 / 128×128)
- 串行标准 Gauss-Seidel
- 串行红黑 Gauss-Seidel
- 并行红黑 Gauss-Seidel（基于区域分解）

### 三维版本 (512×512×512)
- 串行红黑 Gauss-Seidel
- 并行红黑 Gauss-Seidel（基于区域分解 + 多级分块）

## 编译方法

### 编译所有版本
```bash
cd gauss_seidel
make
```

### 二维版本
**使用 Code Runner（推荐）：**
在 VS Code 中打开 `test_gs_2d.cpp`，按 `Ctrl+Alt+N` 运行。

**使用 Makefile：**
```bash
cd gauss_seidel
make run
```

**手动编译：**
```bash
g++ -fopenmp -O2 -std=c++11 test_gs_2d.cpp gauss_seidel_2d.cpp -o test_gs_2d.exe
./test_gs_2d.exe
```

### 三维版本
**使用 Makefile：**
```bash
cd gauss_seidel
make run3d
```

**手动编译：**
```bash
g++ -fopenmp -O2 -std=c++11 test_gs_3d.cpp gauss_seidel_3d.cpp -o test_gs_3d.exe
./test_gs_3d.exe
```

## 文件说明

### 二维版本
- `gauss_seidel_2d.h` - 头文件
- `gauss_seidel_2d.cpp` - 实现文件
- `test_gs_2d.cpp` - 测试程序

### 三维版本
- `gauss_seidel_3d.h` - 头文件
- `gauss_seidel_3d.cpp` - 实现文件
- `test_gs_3d.cpp` - 测试程序

### 其他
- `Makefile` - 编译配置

## 性能结果

### 二维版本 (128×128)

⚠️ **重要提示**：对于小规模问题，可能观察不到明显的并行加速，甚至会变慢。

| 线程数 | 计算时间 | 加速比 |
|-------|---------|--------|
| 1     | 605 ms  | 1.00x  |
| 2     | 1000 ms | 0.61x  |
| 4     | 1094 ms | 0.55x  |
| 8     | 1483 ms | 0.41x  |

**原因分析：**
1. **内存密集型算法**：Gauss-Seidel 主要是内存访问，计算量很小
2. **并行开销**：线程创建、同步、调度的开销 > 计算收益
3. **内存带宽瓶颈**：多线程竞争内存带宽
4. **缓存一致性开销**：多核之间的缓存同步

### 三维版本 (512×512×512)

在大规模三维问题上，并行版本展现出良好的加速效果：

| 线程数 | 计算时间 | 加速比 | 并行效率 |
|-------|---------|--------|---------|
| 1     | 155.4 s | 1.00x  | 100.0%  |
| 2     | 95.3 s  | 1.63x  | 81.5%   |
| 4     | 51.7 s  | 3.01x  | 75.2%   |
| 8     | 32.6 s  | 4.77x  | 59.7%   |

**关键观察：**
- 三维问题规模大（1.34亿格点），并行化效果显著
- 8线程达到4.77倍加速，接近理想线性加速的60%
- 红黑排序消除数据依赖，实现真正的并行计算
- 采用64×64×64分块优化缓存局部性
- `collapse(3)` 指令增加并行粒度，充分利用多核资源

## 实现要点

1. **红黑排序**：(i+j) % 2 或 (i+j+k) % 2 区分红黑点，消除数据依赖
2. **区域分解**：使用 OpenMP 进行多线程并行
3. **多级分块**：提高缓存局部性（2D: 32×32，3D: 64×64×64）
4. **静态调度**：减少线程调度开销
5. **collapse 指令**：增加并行粒度（3D 使用 collapse(3)）
6. **优化残差计算**：减少收敛检查频率（每10次迭代）

## 参考资料

- [gulang2019/optimizing-gauss-seidel-iteration](https://github.com/gulang2019/optimizing-gauss-seidel-iteration.git)
- 采用类似的多级分块策略和并行化技术
- 三维版本使用相同的512×512×512规模进行性能测试
