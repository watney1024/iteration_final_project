#!/bin/bash

echo "========================================"
echo "红黑排序 Gauss-Seidel 算法 - Linux 编译脚本"
echo "========================================"
echo

# 检查是否安装了 g++
if ! command -v g++ &> /dev/null; then
    echo "错误: 未找到 g++ 编译器！"
    echo "请安装 g++ (例如: sudo apt-get install g++)"
    exit 1
fi

echo "检测到 g++ 编译器..."
g++ --version | head -n 1
echo

echo "正在编译正确性测试程序..."
g++ -fopenmp -O3 -std=c++11 test_correctness.cpp red_black_gauss_seidel.cpp -o test_correctness
if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi
echo "编译成功: test_correctness"
echo

echo "正在编译性能测试程序..."
g++ -fopenmp -O3 -std=c++11 test_performance.cpp red_black_gauss_seidel.cpp -o test_performance
if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi
echo "编译成功: test_performance"
echo

echo "========================================"
echo "编译完成！"
echo "========================================"
echo
echo "运行命令:"
echo "  ./test_correctness  - 运行正确性测试"
echo "  ./test_performance  - 运行性能测试"
echo
