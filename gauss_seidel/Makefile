# 编译器和选项
CXX = g++
CXXFLAGS = -fopenmp -O3 -std=c++11 -march=native
CXXFLAGS_OPT = -fopenmp -O3 -std=c++11 -march=native -mavx2 -mfma -funroll-loops

# 目标程序
TARGET_2D = test_gs_2d
TARGET_3D = test_gs_3d
TARGET_2D_BATCH = test_gs_2d_batch
TARGET_3D_BATCH = test_gs_3d_batch
TARGET_MEM_OPT = test_memory_optimization

# 源文件
SOURCES_2D = test_gs_2d.cpp gauss_seidel_2d.cpp
SOURCES_3D = test_gs_3d.cpp gauss_seidel_3d.cpp
SOURCES_2D_BATCH = test_gs_2d_batch.cpp gauss_seidel_2d.cpp
SOURCES_3D_BATCH = test_gs_3d_batch.cpp gauss_seidel_3d.cpp
SOURCES_MEM_OPT = test_memory_optimization.cpp gauss_seidel_2d.cpp gauss_seidel_2d_optimized.cpp

# 默认目标
all: $(TARGET_2D) $(TARGET_3D) $(TARGET_2D_BATCH) $(TARGET_3D_BATCH)

# 优化版本
opt: $(TARGET_MEM_OPT)

# 二维版本
$(TARGET_2D): $(SOURCES_2D)
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 三维版本
$(TARGET_3D): $(SOURCES_3D)
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 二维批处理版本
$(TARGET_2D_BATCH): $(SOURCES_2D_BATCH)
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 三维批处理版本
$(TARGET_3D_BATCH): $(SOURCES_3D_BATCH)
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 访存优化版本（使用激进优化选项）
$(TARGET_MEM_OPT): $(SOURCES_MEM_OPT)
	$(CXX) $(CXXFLAGS_OPT) $^ -o $@.exe

# 清理
clean:
	rm -f *.exe *.o

# 运行二维版本
run_2d: $(TARGET_2D)
	./$(TARGET_2D).exe

# 运行三维版本
run_3d: $(TARGET_3D)
	./$(TARGET_3D).exe

# 运行访存优化测试
run_opt: $(TARGET_MEM_OPT)
	@echo "运行访存优化性能对比测试..."
	./$(TARGET_MEM_OPT).exe 512 8

# 性能分析（需要perf工具，仅Linux）
perf: $(TARGET_MEM_OPT)
	perf stat -e cache-misses,cache-references,L1-dcache-loads,L1-dcache-load-misses \
		./$(TARGET_MEM_OPT).exe 512 8

.PHONY: all opt clean run_2d run_3d run_opt perf
