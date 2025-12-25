# 神经网络算子测试脚本
# 用法: .\test_operators.ps1

Write-Host "======================================" -ForegroundColor Green
Write-Host "  神经网络算子性能测试" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Green

# 设置线程数（可修改：1, 2, 4, 8）
$env:OMP_NUM_THREADS = 8

# 测试1: 卷积算子
Write-Host "`n[1/2] 测试卷积算子 (Conv2d)..." -ForegroundColor Cyan
if (Test-Path "conv.exe") { Remove-Item "conv.exe" }
g++ -fopenmp -O2 -std=c++11 conv.cpp -o conv.exe
if ($LASTEXITCODE -eq 0) {
    Write-Host "编译成功，运行测试..." -ForegroundColor Green
    .\conv.exe
} else {
    Write-Host "编译失败！" -ForegroundColor Red
}

# 测试2: 平均池化算子
Write-Host "`n[2/2] 测试平均池化算子 (AvgPool2d)..." -ForegroundColor Cyan
if (Test-Path "avgpool.exe") { Remove-Item "avgpool.exe" }
g++ -fopenmp -O2 -std=c++11 avgpool.cpp -o avgpool.exe
if ($LASTEXITCODE -eq 0) {
    Write-Host "编译成功，运行测试..." -ForegroundColor Green
    .\avgpool.exe
} else {
    Write-Host "编译失败！" -ForegroundColor Red
}

Write-Host "`n======================================" -ForegroundColor Green
Write-Host "  测试完成！" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Green
