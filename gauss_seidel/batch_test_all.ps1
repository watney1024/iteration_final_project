# 批量测试2D和3D Gauss-Seidel求解器
# 测试原始实现、Tiling优化和串行版本
Set-Location -Path $PSScriptRoot

# 设置UTF-8编码
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

# 创建结果目录
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultDir = "batch_results_$timestamp"
New-Item -ItemType Directory -Force -Path $resultDir | Out-Null

Write-Host "======================================================================"
Write-Host "           Gauss-Seidel 批量性能测试"
Write-Host "======================================================================"
Write-Host "测试时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Write-Host "结果目录: $resultDir"
Write-Host ""

# 编译所有程序
Write-Host ">>> 编译测试程序..."
Write-Host ""

Write-Host "[1/2] 编译2D测试程序..."
$compile2d = "g++ -O2 -fopenmp gauss_seidel_2d.cpp gauss_seidel_2d_tiled.cpp test_tiled_performance.cpp -o test_tiled_2d.exe"
Invoke-Expression $compile2d
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 2D程序编译失败!" -ForegroundColor Red
    exit 1
}
Write-Host "? 2D程序编译成功" -ForegroundColor Green

Write-Host "[2/2] 编译3D测试程序..."
$compile3d = "g++ -O2 -fopenmp gauss_seidel_3d.cpp gauss_seidel_3d_tiled.cpp test_tiled_performance_3d.cpp -o test_tiled_3d.exe"
Invoke-Expression $compile3d
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 3D程序编译失败!" -ForegroundColor Red
    exit 1
}
Write-Host "? 3D程序编译成功" -ForegroundColor Green
Write-Host ""

# 测试参数
$sizes2D = @(64, 128, 256, 512, 1024)
$sizes3D = @(64, 128, 256, 512)
$threads = @(1, 2, 4, 8, 10, 16, 20)

# ====================================================================
# 2D测试
# ====================================================================
Write-Host "======================================================================"
Write-Host "                        2D 泊松方程测试"
Write-Host "======================================================================"
Write-Host ""

$results2D = @()

foreach ($N in $sizes2D) {
    Write-Host "----------------------------------------------------------------------"
    Write-Host "测试规模: ${N}x${N}"
    Write-Host "----------------------------------------------------------------------"
    
    # 用于存储单线程基准时间
    $baseline_orig = 0
    $baseline_tiled = 0
    
    # 临时存储本规模的所有结果
    $temp_results = @()
    
    foreach ($t in $threads) {
        # 运行测试并捕获输出
        $output = & .\test_tiled_2d.exe 2d $N $t 2>&1 | Out-String
        
        # 解析结果
        $time_orig = 0
        $time_tiled = 0
        $iter_orig = 0
        $iter_tiled = 0
        $error_orig = "N/A"
        $error_tiled = "N/A"
        
        if ($output -match "Original\s+\|\s+(\d+)\s+\|\s+([\d.]+)\s+\|\s+([\d.e+-]+)\s+\|\s+([\d.]+)x") {
            $iter_orig = $matches[1]
            $time_orig = [double]$matches[2]
            $error_orig = $matches[3]
        }
        
        if ($output -match "2-Layer Tiling\s+\|\s+(\d+)\s+\|\s+([\d.]+)\s+\|\s+([\d.e+-]+)\s+\|\s+([\d.]+)x") {
            $iter_tiled = $matches[1]
            $time_tiled = [double]$matches[2]
            $error_tiled = $matches[3]
        }
        
        # 记录单线程基准时间
        if ($t -eq 1) {
            $baseline_orig = $time_orig
            $baseline_tiled = $time_tiled
        }
        
        # 计算Original相对于Original(1线程)的加速比
        $speedup_orig = 1.0
        $efficiency_orig = 100.0
        if ($baseline_orig -gt 0 -and $time_orig -gt 0) {
            $speedup_orig = [math]::Round($baseline_orig / $time_orig, 2)
            $efficiency_orig = [math]::Round(($speedup_orig / $t) * 100, 1)
        }
        
        # 计算Tiled相对于Tiled(1线程)的加速比
        $speedup_tiled = 1.0
        $efficiency_tiled = 100.0
        if ($baseline_tiled -gt 0 -and $time_tiled -gt 0) {
            $speedup_tiled = [math]::Round($baseline_tiled / $time_tiled, 2)
            $efficiency_tiled = [math]::Round(($speedup_tiled / $t) * 100, 1)
        }
        
        # 记录Original结果
        $result_orig = [PSCustomObject]@{
            Dim = "2D"
            Size = $N
            Threads = $t
            Method = "Original"
            Time = $time_orig
            Speedup = $speedup_orig
            Efficiency = $efficiency_orig
            Iterations = $iter_orig
            Error = $error_orig
        }
        $temp_results += $result_orig
        
        # 记录Tiled结果
        $result_tiled = [PSCustomObject]@{
            Dim = "2D"
            Size = $N
            Threads = $t
            Method = "Tiled"
            Time = $time_tiled
            Speedup = $speedup_tiled
            Efficiency = $efficiency_tiled
            Iterations = $iter_tiled
            Error = $error_tiled
        }
        $temp_results += $result_tiled
        
        # 保存详细输出
        $outputFile = Join-Path $resultDir "2d_N${N}_t${t}.txt"
        $output | Out-File -FilePath $outputFile -Encoding UTF8
    }
    
    # 显示结果：先显示所有Original
    Write-Host ("{0,-20} {1,12} {2,10} {3,10}" -f "配置", "时间(ms)", "加速比", "效率(%)")
    Write-Host ("{0,-20} {1,12} {2,10} {3,10}" -f "--------------------", "-----------", "--------", "--------")
    Write-Host "Original 方法:" -ForegroundColor Yellow
    foreach ($r in ($temp_results | Where-Object { $_.Method -eq "Original" })) {
        Write-Host ("{0,-20} {1,12:F2} {2,9}x {3,9}%" -f "$($r.Threads) 线程", $r.Time, $r.Speedup, $r.Efficiency) -ForegroundColor Cyan
        $results2D += $r
    }
    
    Write-Host ""
    Write-Host "Tiled 方法:" -ForegroundColor Yellow
    foreach ($r in ($temp_results | Where-Object { $_.Method -eq "Tiled" })) {
        Write-Host ("{0,-20} {1,12:F2} {2,9}x {3,9}%" -f "$($r.Threads) 线程", $r.Time, $r.Speedup, $r.Efficiency) -ForegroundColor Green
        $results2D += $r
    }
    Write-Host ""
}

# 保存2D汇总结果
$summary2D = Join-Path $resultDir "summary_2d.csv"
$results2D | Export-Csv -Path $summary2D -NoTypeInformation -Encoding UTF8
Write-Host "? 2D测试完成，结果已保存到: $summary2D" -ForegroundColor Green
Write-Host ""

# ====================================================================
# 3D测试
# ====================================================================
Write-Host "======================================================================"
Write-Host "                        3D 泊松方程测试"
Write-Host "======================================================================"
Write-Host ""

$results3D = @()

foreach ($N in $sizes3D) {
    Write-Host "----------------------------------------------------------------------"
    Write-Host "测试规模: ${N}x${N}x${N}"
    Write-Host "----------------------------------------------------------------------"
    
    # 用于存储单线程基准时间
    $baseline_orig = 0
    $baseline_tiled = 0
    
    # 临时存储本规模的所有结果
    $temp_results = @()
    
    foreach ($t in $threads) {
        # 运行测试并捕获输出
        $output = & .\test_tiled_3d.exe $N $t 2>&1 | Out-String
        
        # 解析结果
        $time_orig = 0
        $time_tiled = 0
        $iter_orig = 0
        $iter_tiled = 0
        $error_orig = "N/A"
        $error_tiled = "N/A"
        
        if ($output -match "Original\s+\|\s+(\d+)\s+\|\s+([\d.]+)\s+\|\s+([\d.e+-]+)\s+\|\s+([\d.]+)x") {
            $iter_orig = $matches[1]
            $time_orig = [double]$matches[2]
            $error_orig = $matches[3]
        }
        
        if ($output -match "2-Layer Tiling\s+\|\s+(\d+)\s+\|\s+([\d.]+)\s+\|\s+([\d.e+-]+)\s+\|\s+([\d.]+)x") {
            $iter_tiled = $matches[1]
            $time_tiled = [double]$matches[2]
            $error_tiled = $matches[3]
        }
        
        # 记录单线程基准时间
        if ($t -eq 1) {
            $baseline_orig = $time_orig
            $baseline_tiled = $time_tiled
        }
        
        # 计算Original相对于Original(1线程)的加速比
        $speedup_orig = 1.0
        $efficiency_orig = 100.0
        if ($baseline_orig -gt 0 -and $time_orig -gt 0) {
            $speedup_orig = [math]::Round($baseline_orig / $time_orig, 2)
            $efficiency_orig = [math]::Round(($speedup_orig / $t) * 100, 1)
        }
        
        # 计算Tiled相对于Tiled(1线程)的加速比
        $speedup_tiled = 1.0
        $efficiency_tiled = 100.0
        if ($baseline_tiled -gt 0 -and $time_tiled -gt 0) {
            $speedup_tiled = [math]::Round($baseline_tiled / $time_tiled, 2)
            $efficiency_tiled = [math]::Round(($speedup_tiled / $t) * 100, 1)
        }
        
        # 记录Original结果
        $result_orig = [PSCustomObject]@{
            Dim = "3D"
            Size = $N
            Threads = $t
            Method = "Original"
            Time = $time_orig
            Speedup = $speedup_orig
            Efficiency = $efficiency_orig
            Iterations = $iter_orig
            Error = $error_orig
        }
        $temp_results += $result_orig
        
        # 记录Tiled结果
        $result_tiled = [PSCustomObject]@{
            Dim = "3D"
            Size = $N
            Threads = $t
            Method = "Tiled"
            Time = $time_tiled
            Speedup = $speedup_tiled
            Efficiency = $efficiency_tiled
            Iterations = $iter_tiled
            Error = $error_tiled
        }
        $temp_results += $result_tiled
        
        # 保存详细输出
        $outputFile = Join-Path $resultDir "3d_N${N}_t${t}.txt"
        $output | Out-File -FilePath $outputFile -Encoding UTF8
    }
    
    # 显示结果：先显示所有Original
    Write-Host ("{0,-20} {1,12} {2,10} {3,10}" -f "配置", "时间(ms)", "加速比", "效率(%)")
    Write-Host ("{0,-20} {1,12} {2,10} {3,10}" -f "--------------------", "-----------", "--------", "--------")
    Write-Host "Original 方法:" -ForegroundColor Yellow
    foreach ($r in ($temp_results | Where-Object { $_.Method -eq "Original" })) {
        Write-Host ("{0,-20} {1,12:F2} {2,9}x {3,9}%" -f "$($r.Threads) 线程", $r.Time, $r.Speedup, $r.Efficiency) -ForegroundColor Cyan
        $results3D += $r
    }
    
    Write-Host ""
    Write-Host "Tiled 方法:" -ForegroundColor Yellow
    foreach ($r in ($temp_results | Where-Object { $_.Method -eq "Tiled" })) {
        Write-Host ("{0,-20} {1,12:F2} {2,9}x {3,9}%" -f "$($r.Threads) 线程", $r.Time, $r.Speedup, $r.Efficiency) -ForegroundColor Green
        $results3D += $r
    }
    Write-Host ""
}

# 保存3D汇总结果
$summary3D = Join-Path $resultDir "summary_3d.csv"
$results3D | Export-Csv -Path $summary3D -NoTypeInformation -Encoding UTF8
Write-Host "? 3D测试完成，结果已保存到: $summary3D" -ForegroundColor Green
Write-Host ""
# ====================================================================
# 生成性能分析报告
# ====================================================================
Write-Host "======================================================================"
Write-Host "                       生成性能分析报告"
Write-Host "======================================================================"
Write-Host ""

$reportFile = Join-Path $resultDir "performance_report.txt"

@"
====================================================================
    Gauss-Seidel 并行优化性能测试报告
====================================================================
测试时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
测试配置:
  编译器: g++ -O2 -fopenmp
  2D测试规模: $($sizes2D -join ', ')
  3D测试规模: $($sizes3D -join ', ')
  线程数: $($threads -join ', ')

====================================================================
                        2D 性能汇总
====================================================================

"@ | Out-File -FilePath $reportFile -Encoding UTF8

# 2D最佳加速比（分别统计Original和Tiled）
foreach ($N in $sizes2D) {
    $best_orig = $results2D | Where-Object { $_.Size -eq $N -and $_.Method -eq "Original" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1
    $best_tiled = $results2D | Where-Object { $_.Size -eq $N -and $_.Method -eq "Tiled" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1
    if ($best_orig) {
        "N=$N Original最佳: $($best_orig.Threads)线程, 加速比=$($best_orig.Speedup)x" | Out-File -FilePath $reportFile -Append -Encoding UTF8
    }
    if ($best_tiled) {
        "N=$N Tiled最佳: $($best_tiled.Threads)线程, 加速比=$($best_tiled.Speedup)x" | Out-File -FilePath $reportFile -Append -Encoding UTF8
    }
}

@"

====================================================================
                        3D 性能汇总
====================================================================

"@ | Out-File -FilePath $reportFile -Append -Encoding UTF8

# 3D最佳加速比（分别统计Original和Tiled）
foreach ($N in $sizes3D) {
    $best_orig = $results3D | Where-Object { $_.Size -eq $N -and $_.Method -eq "Original" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1
    $best_tiled = $results3D | Where-Object { $_.Size -eq $N -and $_.Method -eq "Tiled" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1
    if ($best_orig) {
        "N=$N Original最佳: $($best_orig.Threads)线程, 加速比=$($best_orig.Speedup)x" | Out-File -FilePath $reportFile -Append -Encoding UTF8
    }
    if ($best_tiled) {
        "N=$N Tiled最佳: $($best_tiled.Threads)线程, 加速比=$($best_tiled.Speedup)x" | Out-File -FilePath $reportFile -Append -Encoding UTF8
    }
}

@"

====================================================================
                        关键发现
====================================================================

"@ | Out-File -FilePath $reportFile -Append -Encoding UTF8

# 分析并输出关键发现
$best2D_orig = $results2D | Where-Object { $_.Method -eq "Original" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1
$best2D_tiled = $results2D | Where-Object { $_.Method -eq "Tiled" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1
$best3D_orig = $results3D | Where-Object { $_.Method -eq "Original" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1
$best3D_tiled = $results3D | Where-Object { $_.Method -eq "Tiled" } | Sort-Object -Property Speedup -Descending | Select-Object -First 1

@"
1. 2D最佳性能:
   Original:
     规模: $($best2D_orig.Size)x$($best2D_orig.Size)
     线程数: $($best2D_orig.Threads)
     加速比: $($best2D_orig.Speedup)x
     时间: $($best2D_orig.Time)ms
   
   Tiled:
     规模: $($best2D_tiled.Size)x$($best2D_tiled.Size)
     线程数: $($best2D_tiled.Threads)
     加速比: $($best2D_tiled.Speedup)x
     时间: $($best2D_tiled.Time)ms

2. 3D最佳性能:
   Original:
     规模: $($best3D_orig.Size)x$($best3D_orig.Size)x$($best3D_orig.Size)
     线程数: $($best3D_orig.Threads)
     加速比: $($best3D_orig.Speedup)x
     时间: $($best3D_orig.Time)ms
   
   Tiled:
     规模: $($best3D_tiled.Size)x$($best3D_tiled.Size)x$($best3D_tiled.Size)
     线程数: $($best3D_tiled.Threads)
     加速比: $($best3D_tiled.Speedup)x
     时间: $($best3D_tiled.Timet3D.Size)x$($best3D.Size)
   线程数: $($best3D.Threads)
   加速比: $($best3D.Speedup)x
   原始时间: $($best3D.Time_Orig)ms
   优化时间: $($best3D.Time_Tiled)ms

3. 优化技术:
   2层Tiling分块策略
   红黑排序消除数据依赖
   collapse(3)增加3D并行粒度
   寄存器缓存减少内存访问
   自适应check_interval减少同步开销
" -ForegroundColor Yellow
Write-Host "  Original: N=$($best2D_orig.Size), $($best2D_orig.Threads)线程, 加速比=$($best2D_orig.Speedup)x" -ForegroundColor Cyan
Write-Host "  Tiled:    N=$($best2D_tiled.Size), $($best2D_tiled.Threads)线程, 加速比=$($best2D_tiled.Speedup)x" -ForegroundColor Green
Write-Host ""
Write-Host "3D最佳性能:" -ForegroundColor Yellow
Write-Host "  Original: N=$($best3D_orig.Size), $($best3D_orig.Threads)线程, 加速比=$($best3D_orig.Speedup)x" -ForegroundColor Cyan
Write-Host "  Tiled:    N=$($best3D_tiled.Size), $($best3D_tiled.Threads)线程, 加速比=$($best3D_tiled.Speedup)x" -ForegroundColor Green
                        测试完成
====================================================================
所有结果已保存到目录: $resultDir

"@ | Out-File -FilePath $reportFile -Append -Encoding UTF8

Write-Host "? 性能报告已生成: $reportFile" -ForegroundColor Green
Write-Host ""

# 显示报告摘要
Write-Host "======================================================================"
Write-Host "                         测试总结"
Write-Host "======================================================================"
Write-Host ""
Write-Host "2D最佳性能: N=$($best2D.Size), $($best2D.Threads)线程, 加速比=$($best2D.Speedup)x" -ForegroundColor Yellow
Write-Host "3D最佳性能: N=$($best3D.Size), $($best3D.Threads)线程, 加速比=$($best3D.Speedup)x" -ForegroundColor Yellow
Write-Host ""
Write-Host "详细结果请查看: $resultDir" -ForegroundColor Cyan
Write-Host "======================================================================"
Write-Host ""
