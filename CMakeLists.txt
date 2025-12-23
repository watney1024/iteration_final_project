cmake_minimum_required(VERSION 3.10)
project(GaussSeidel2D)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 OpenMP 支持
find_package(OpenMP REQUIRED)

# 添加可执行文件
add_executable(test_gs_2d 
    test_gs_2d.cpp
    gauss_seidel_2d.cpp
)

# 链接 OpenMP
target_link_libraries(test_gs_2d PUBLIC OpenMP::OpenMP_CXX)

# 设置优化选项
target_compile_options(test_gs_2d PRIVATE -O2)
