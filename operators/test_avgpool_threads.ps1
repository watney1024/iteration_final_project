# 批量测试不同线程数的avgpool_openmp程序
# 使用方法: .\test_avgpool_threads.ps1

# 切换到脚本所在目录
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "AvgPool OpenMP Multi-Thread Performance Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 定义测试的线程数
$threadCounts = @(1, 2, 4, 8, 10, 16, 20)

# # 先测试串行版本
# Write-Host "Compiling avgpool.cpp (Serial version)..." -ForegroundColor Yellow
# g++ -O2 -std=c++11 avgpool.cpp -o avgpool.exe

# if ($LASTEXITCODE -eq 0) {
#     Write-Host "Compilation successful!" -ForegroundColor Green
#     Write-Host "Running serial version..." -ForegroundColor Cyan
#     $serialOutput = .\avgpool.exe
#     Write-Host $serialOutput -ForegroundColor White
#     Write-Host ""
# } else {
#     Write-Host "Serial compilation failed!" -ForegroundColor Red
# }

# 编译OpenMP版本
Write-Host "Compiling avgpool_openmp.cpp..." -ForegroundColor Yellow
g++ -fopenmp -O2 -std=c++11 avgpool_openmp.cpp -o avgpool_openmp.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Compilation successful!" -ForegroundColor Green
Write-Host ""

# 创建结果文件
$resultFile = "avgpool_openmp_results.txt"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
"AvgPool OpenMP Performance Test Results" | Out-File -FilePath $resultFile
"Test Date: $timestamp" | Out-File -FilePath $resultFile -Append
"Input: 1x320x300x300, Output: 1x320x150x150" | Out-File -FilePath $resultFile -Append
"Kernel: 2x2, Stride: 2x2" | Out-File -FilePath $resultFile -Append
"==========================================" | Out-File -FilePath $resultFile -Append
"" | Out-File -FilePath $resultFile -Append

# 保存串行版本结果
if ($serialOutput) {
    "Serial Version:" | Out-File -FilePath $resultFile -Append
    $serialOutput | Out-File -FilePath $resultFile -Append
    "" | Out-File -FilePath $resultFile -Append
}

Write-Host "Starting parallel tests..." -ForegroundColor Yellow
Write-Host ""

foreach ($threads in $threadCounts) {
    Write-Host "Testing with $threads thread(s)..." -ForegroundColor Cyan
    
    # 设置OpenMP线程数
    $env:OMP_NUM_THREADS = $threads
    
    # 运行程序
    $output = .\avgpool_openmp.exe
    
    # 输出到控制台
    Write-Host "Threads: $threads" -ForegroundColor Yellow
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
