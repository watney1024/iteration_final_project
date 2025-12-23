@echo off
echo ========================================
echo 红黑排序 Gauss-Seidel 算法 - Windows 编译脚本
echo ========================================
echo.

REM 检查是否安装了 g++
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo 错误: 未找到 g++ 编译器！
    echo 请安装 MinGW 或 MSYS2 并确保 g++ 在 PATH 中。
    pause
    exit /b 1
)

echo 检测到 g++ 编译器...
g++ --version | findstr /C:"g++"
echo.

echo 正在编译正确性测试程序...
g++ -fopenmp -O3 -std=c++11 test_correctness.cpp red_black_gauss_seidel.cpp -o test_correctness.exe
if %errorlevel% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)
echo 编译成功: test_correctness.exe
echo.

echo 正在编译性能测试程序...
g++ -fopenmp -O3 -std=c++11 test_performance.cpp red_black_gauss_seidel.cpp -o test_performance.exe
if %errorlevel% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)
echo 编译成功: test_performance.exe
echo.

echo ========================================
echo 编译完成！
echo ========================================
echo.
echo 运行命令:
echo   test_correctness.exe  - 运行正确性测试
echo   test_performance.exe  - 运行性能测试
echo.
pause
