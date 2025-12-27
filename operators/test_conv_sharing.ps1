# 批量测试不同线程数的conv_openmp_sharing程序（优化版本）
# 使用方法: .\test_conv_memory.ps1

# 切换到脚本所在目录
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Conv OpenMP Memory Optimized Performance Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 定义测试的线程数
$threadCounts = @(1, 2, 4, 8, 10, 16, 20)

# 编译程序
Write-Host "Compiling conv_openmp_sharing.cpp..." -ForegroundColor Yellow
g++ -fopenmp -O2 -std=c++11 conv_openmp_sharing.cpp -o conv_openmp_sharing.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Compilation successful!" -ForegroundColor Green
Write-Host ""

# 创建结果文件
$resultFile = "conv_openmp_sharing_results.txt"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
"Conv OpenMP Memory Optimized Performance Test Results" | Out-File -FilePath $resultFile
"Test Date: $timestamp" | Out-File -FilePath $resultFile -Append
"Optimizations: Memory management + Access pattern + Parallel optimization" | Out-File -FilePath $resultFile -Append
"==========================================" | Out-File -FilePath $resultFile -Append
"" | Out-File -FilePath $resultFile -Append

Write-Host "Starting tests..." -ForegroundColor Yellow
Write-Host ""

foreach ($threads in $threadCounts) {
    Write-Host "Testing with $threads thread(s)..." -ForegroundColor Cyan
    
    # 运行程序
    $output = .\conv_openmp_sharing.exe $threads
    
    # 输出到控制台
    Write-Host $output -ForegroundColor White
    
    # 保存到文件
    "Threads: $threads" | Out-File -FilePath $resultFile -Append
    $output | Out-File -FilePath $resultFile -Append
    "" | Out-File -FilePath $resultFile -Append
    
    Write-Host ""
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "All tests completed!" -ForegroundColor Green
Write-Host "Results saved to: $resultFile" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
