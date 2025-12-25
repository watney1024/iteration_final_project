# 测试脚本使用说明

## ⚠️ 重要：需要安装g++编译器

所有脚本都需要g++编译器。如果你没有安装，请先安装：

### 安装MinGW (推荐)
1. 下载: https://sourceforge.net/projects/mingw/
2. 安装时选择 `mingw32-gcc-g++`
3. 添加到PATH: `C:\MinGW\bin`
4. 重启终端测试: `g++ --version`

### 或安装MSYS2
1. 下载: https://www.msys2.org/
2. 安装后运行: `pacman -S mingw-w64-x86_64-gcc`
3. 添加到PATH: `C:\msys64\mingw64\bin`

---

## 三种运行方式（按推荐顺序）

### 🥇 方式1: Windows批处理文件（最简单）
- **文件**: `test_memory_opt.bat`
- **优点**: 双击即可运行，无需PowerShell权限
- **推荐**: Windows用户首选

```cmd
# 直接双击 test_memory_opt.bat
# 或在命令行运行
test_memory_opt.bat
```

### 🥈 方式2: PowerShell脚本（无需make）
- **文件**: `test_memory_opt_windows.ps1`
- **优点**: 不需要make工具，直接用g++编译
- **推荐**: 有PowerShell经验的用户

```powershell
# 切换到gauss_seidel目录
cd gauss_seidel

# 运行脚本
powershell -ExecutionPolicy ByPass -File ".\test_memory_opt_windows.ps1"
```

### 🥉 方式3: 传统PowerShell（需要make）
- **文件**: `test_memory_opt.ps1` 或 `test_memory_opt_cn.ps1`
- **要求**: 需要安装make工具（通过MSYS2或MinGW）
- **适用**: 已有完整编译环境的用户

```powershell
.\test_memory_opt.ps1  # 英文版
# 或
.\test_memory_opt_cn.ps1  # 中文版
```

## 手动测试（如果脚本失败）

如果PowerShell脚本运行失败，可以手动执行：

```powershell
# 1. 切换到gauss_seidel目录
cd gauss_seidel

# 2. 编译
make opt

# 3. 运行测试（手动指定参数）
.\test_memory_optimization.exe 256 4   # 256x256, 4线程
.\test_memory_optimization.exe 512 8   # 512x512, 8线程
.\test_memory_optimization.exe 1024 16 # 1024x1024, 16线程
```

## 常见问题

### Q: "make: command not found" 或 "无法将make项识别"？
**A**: 你的系统没有make工具。请使用：
- **方式1**: 批处理文件 `test_memory_opt.bat`（推荐）
- **方式2**: Windows版PowerShell `test_memory_opt_windows.ps1`

这两种方式不需要make！

### Q: "g++: command not found" 或 "无法将g++项识别"？
**A**: 你需要安装g++编译器（见上方安装说明）。没有g++就无法编译C++代码。

### Q: 编译时出现AVX2错误？
**A**: 脚本会自动尝试不带AVX2编译。如果还是失败，手动运行：
```powershell
cd gauss_seidel
g++ -O2 -fopenmp gauss_seidel_2d.cpp gauss_seidel_2d_optimized.cpp test_memory_optimization.cpp -o test_memory_optimization.exe
```

## 直接运行测试（不用脚本）

如果所有脚本都失败，最简单的方法：

```powershell
# 编译（一次性）
make opt

# 单次测试
.\test_memory_optimization.exe 512 8

# 批量测试（复制粘贴所有行一起运行）
.\test_memory_optimization.exe 256 1
.\test_memory_optimization.exe 256 4
.\test_memory_optimization.exe 256 8
.\test_memory_optimization.exe 512 1
.\test_memory_optimization.exe 512 4
.\test_memory_optimization.exe 512 8
.\test_memory_optimization.exe 1024 1
.\test_memory_optimization.exe 1024 4
.\test_memory_optimization.exe 1024 8
```

## 输出示例

成功运行后会看到：

```
============================================================
方法: 基准: 当前实现 (无分块)
------------------------------------------------------------
网格规模:        512 x 512
迭代次数:        342
最终残差:        9.876543e-07
相对误差:        1.234567e-06
计算时间:        186.234 ms
每次迭代时间:    0.544 ms
============================================================

============================================================
方法: 优化1: 二级Tiling + 预取 + 数据重用
------------------------------------------------------------
网格规模:        512 x 512
迭代次数:        342
最终残差:        9.876543e-07
相对误差:        1.234567e-06
计算时间:        92.156 ms
每次迭代时间:    0.269 ms
加速比:          2.02x
性能提升:        50.5%
============================================================
```

## 技术支持

如果遇到其他问题，请查看：
- 详细报告: [MEMORY_OPTIMIZATION_REPORT.md](MEMORY_OPTIMIZATION_REPORT.md)
- 使用指南: [OPTIMIZATION_GUIDE.md](OPTIMIZATION_GUIDE.md)
