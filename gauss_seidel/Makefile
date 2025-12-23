# 编译器和选项
CXX = g++
CXXFLAGS = -fopenmp -O2 -std=c++11

# 目标程序
TARGET_2D = test_gs_2d
TARGET_3D = test_gs_3d

# 源文件
SOURCES_2D = test_gs_2d.cpp gauss_seidel_2d.cpp
SOURCES_3D = test_gs_3d.cpp gauss_seidel_3d.cpp

# 默认目标
all: $(TARGET_2D) $(TARGET_3D)

# 二维版本
$(TARGET_2D): $(SOURCES_2D)
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 三维版本
$(TARGET_3D): $(SOURCES_3D)
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 清理
clean:
	rm -f $(TARGET_2D).exe $(TARGET_3D).exe *.o

# 运行二维版本
run: $(TARGET_2D)
	./$(TARGET_2D).exe

# 运行三维版本
run3d: $(TARGET_3D)
	./$(TARGET_3D).exe

.PHONY: all clean run run3d
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f *.o $(TARGET).exe

# 运行
run: $(TARGET)
	./$(TARGET).exe

.PHONY: all clean run
