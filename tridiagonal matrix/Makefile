# 编译器和选项
CXX = g++
CXXFLAGS = -fopenmp -O2 -std=c++11

# 目标程序
TARGET = test_tdma

# 源文件
SOURCES = test_tdma.cpp tdma_solver.cpp

# 默认目标
all: $(TARGET)

# 编译
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 清理
clean:
	rm -f $(TARGET).exe *.o

# 运行
run: $(TARGET)
	./$(TARGET).exe

.PHONY: all clean run
