# 内存对齐优化批量测试脚本
# 测试Original、Tiled和Tiled+Aligned三个版本
Set-Location -Path $PSScriptRoot
Write-Host "========== Memory Alignment Optimization Batch Test ==========" -ForegroundColor Cyan
Write-Host "Compiling test programs..." -ForegroundColor Yellow

# 编译2D测试
Write-Host "`n[1/2] Compiling 2D alignment test..." -ForegroundColor Green
g++ -O2 -march=native -fopenmp `
    gauss_seidel_2d.cpp `
    gauss_seidel_2d_tiled.cpp `
    gauss_seidel_2d_tiled_aligned.cpp `
    test_tiled_aligned_2d.cpp `
    -o test_aligned_2d.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "2D compilation failed!" -ForegroundColor Red
    exit 1
}

# 编译3D测试
Write-Host "[2/2] Compiling 3D alignment test..." -ForegroundColor Green
g++ -O2 -march=native -fopenmp `
    gauss_seidel_3d.cpp `
    gauss_seidel_3d_tiled.cpp `
    gauss_seidel_3d_tiled_aligned.cpp `
    test_tiled_aligned_3d.cpp `
    -o test_aligned_3d.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "3D compilation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "`nCompilation successful!" -ForegroundColor Green

# 创建结果目录
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$result_dir = "aligned_results_$timestamp"
New-Item -ItemType Directory -Force -Path $result_dir | Out-Null

# 测试配置
$sizes_2d = @(64, 128, 256, 512, 1024, 2048)
$sizes_3d = @(64, 128, 256, 512)
$thread_counts = @(1, 2, 4, 8, 10, 16, 20)

# 2D测试
Write-Host "`n========== Running 2D Alignment Tests ==========" -ForegroundColor Cyan
$results_2d = @()

foreach ($N in $sizes_2d) {
    Write-Host "`nTesting N=$N..." -ForegroundColor Yellow
    
    foreach ($threads in $thread_counts) {
        Write-Host "  Threads=$threads" -NoNewline
        
        $output = .\test_aligned_2d.exe $N $threads 2>&1 | Out-String
        
        # 解析输出
        $lines = $output -split "`n"
        foreach ($line in $lines) {
            if ($line -match '(Original|Tiled|Tiled\+Aligned)\s+\|\s+(\d+)\s+\|\s+([\d.]+)\s+\|\s+([\d.e+-]+)\s+\|\s+([\d.]+)x') {
                $method = $matches[1]
                $iters = $matches[2]
                $time = $matches[3]
                $err_val = $matches[4]
                $speedup = $matches[5]
                
                $results_2d += [PSCustomObject]@{
                    Dimension = "2D"
                    N = $N
                    Threads = $threads
                    Method = $method
                    Iterations = $iters
                    Time_ms = $time
                    Error = $err_val
                    Speedup = $speedup
                }
            }
        }
        
        Write-Host " - Done" -ForegroundColor Gray
    }
}

# 3D测试
Write-Host "`n========== Running 3D Alignment Tests ==========" -ForegroundColor Cyan
$results_3d = @()

foreach ($N in $sizes_3d) {
    Write-Host "`nTesting N=$N..." -ForegroundColor Yellow
    
    foreach ($threads in $thread_counts) {
        Write-Host "  Threads=$threads" -NoNewline
        
        $output = .\test_aligned_3d.exe $N $threads 2>&1 | Out-String
        
        # 解析输出
        $lines = $output -split "`n"
        foreach ($line in $lines) {
            if ($line -match '(Original|Tiled|Tiled\+Aligned)\s+\|\s+(\d+)\s+\|\s+([\d.]+)\s+\|\s+([\d.e+-]+)\s+\|\s+([\d.]+)x') {
                $method = $matches[1]
                $iters = $matches[2]
                $time = $matches[3]
                $err_val = $matches[4]
                $speedup = $matches[5]
                
                $results_3d += [PSCustomObject]@{
                    Dimension = "3D"
                    N = $N
                    Threads = $threads
                    Method = $method
                    Iterations = $iters
                    Time_ms = $time
                    Error = $err_val
                    Speedup = $speedup
                }
            }
        }
        
        Write-Host " - Done" -ForegroundColor Gray
    }
}

# 保存CSV结果
Write-Host "`n========== Saving Results ==========" -ForegroundColor Cyan

$results_2d | Export-Csv -Path "$result_dir\results_2d.csv" -NoTypeInformation
$results_3d | Export-Csv -Path "$result_dir\results_3d.csv" -NoTypeInformation

Write-Host "Results saved to $result_dir/" -ForegroundColor Green

# 性能分析
Write-Host "`n========== Performance Summary ==========" -ForegroundColor Cyan

# 2D分析
foreach ($N in $sizes_2d) {
    Write-Host "`n" -NoNewline
    Write-Host "2D 泊松方程测试" -ForegroundColor Cyan
    Write-Host "======================================================================"
    Write-Host ""
    Write-Host "----------------------------------------------------------------------"
    Write-Host "测试规模: ${N}x${N}"
    Write-Host "----------------------------------------------------------------------"
    Write-Host ("{0,-20}  {1,11}   {2,8}   {3,8}" -f "配置", "时间(ms)", "加速比", "效率(%)")
    Write-Host ("{0,-20}  {1,11}   {2,8}   {3,8}" -f "--------------------", "-----------", "--------", "--------")
    
    foreach ($method in @("Original", "Tiled", "Tiled+Aligned")) {
        $method_data = $results_2d | Where-Object { $_.N -eq $N -and $_.Method -eq $method } | Sort-Object { [int]$_.Threads }
        
        if ($method_data.Count -gt 0) {
            $baseline = [double]($method_data | Where-Object { $_.Threads -eq 1 }).Time_ms
            
            Write-Host "$method 方法:"
            foreach ($row in $method_data) {
                $time = [double]$row.Time_ms
                $speedup = $baseline / $time
                $efficiency = ($speedup / [int]$row.Threads) * 100
                
                $speedup_str = if ($speedup -ge 1) { "{0:F2}x" -f $speedup } else { "{0:F2}x" -f $speedup }
                
                Write-Host ("{0,-20}  {1,11:F2}   {2,8}   {3,7:F1}%" -f `
                    "$($row.Threads) 线程", $time, $speedup_str, $efficiency)
            }
            Write-Host ""
        }
    }
}

# 3D分析
foreach ($N in $sizes_3d) {
    Write-Host "`n" -NoNewline
    Write-Host "3D 泊松方程测试" -ForegroundColor Cyan
    Write-Host "======================================================================"
    Write-Host ""
    Write-Host "----------------------------------------------------------------------"
    Write-Host "测试规模: ${N}x${N}x${N}"
    Write-Host "----------------------------------------------------------------------"
    Write-Host ("{0,-20}  {1,11}   {2,8}   {3,8}" -f "配置", "时间(ms)", "加速比", "效率(%)")
    Write-Host ("{0,-20}  {1,11}   {2,8}   {3,8}" -f "--------------------", "-----------", "--------", "--------")
    
    foreach ($method in @("Original", "Tiled", "Tiled+Aligned")) {
        $method_data = $results_3d | Where-Object { $_.N -eq $N -and $_.Method -eq $method } | Sort-Object { [int]$_.Threads }
        
        if ($method_data.Count -gt 0) {
            $baseline = [double]($method_data | Where-Object { $_.Threads -eq 1 }).Time_ms
            
            Write-Host "$method 方法:"
            foreach ($row in $method_data) {
                $time = [double]$row.Time_ms
                $speedup = $baseline / $time
                $efficiency = ($speedup / [int]$row.Threads) * 100
                
                $speedup_str = if ($speedup -ge 1) { "{0:F2}x" -f $speedup } else { "{0:F2}x" -f $speedup }
                
                Write-Host ("{0,-20}  {1,11:F2}   {2,8}   {3,7:F1}%" -f `
                    "$($row.Threads) 线程", $time, $speedup_str, $efficiency)
            }
            Write-Host ""
        }
    }
}

# 对齐效果分析
Write-Host "`n`n========== Alignment Impact Analysis ==========" -ForegroundColor Cyan

Write-Host "`n2D Tiled vs Tiled+Aligned (at best thread count):" -ForegroundColor Yellow
foreach ($N in $sizes_2d) {
    $tiled = $results_2d | Where-Object { $_.N -eq $N -and $_.Method -eq "Tiled" } | 
             Sort-Object { [double]$_.Time_ms } | Select-Object -First 1
    $aligned = $results_2d | Where-Object { $_.N -eq $N -and $_.Method -eq "Tiled+Aligned" } |
               Sort-Object { [double]$_.Time_ms } | Select-Object -First 1
    
    if ($tiled -and $aligned) {
        $improvement = ([double]$tiled.Time_ms - [double]$aligned.Time_ms) / [double]$tiled.Time_ms * 100
        $sign = if ($improvement -gt 0) { "+" } else { "" }
        Write-Host ("  N={0}: {1}{2:F1}% ({3}ms vs {4}ms)" -f `
            $N, $sign, $improvement, $tiled.Time_ms, $aligned.Time_ms) -ForegroundColor Gray
    }
}

Write-Host "`n3D Tiled vs Tiled+Aligned (at best thread count):" -ForegroundColor Yellow
foreach ($N in $sizes_3d) {
    $tiled = $results_3d | Where-Object { $_.N -eq $N -and $_.Method -eq "Tiled" } |
             Sort-Object { [double]$_.Time_ms } | Select-Object -First 1
    $aligned = $results_3d | Where-Object { $_.N -eq $N -and $_.Method -eq "Tiled+Aligned" } |
               Sort-Object { [double]$_.Time_ms } | Select-Object -First 1
    
    if ($tiled -and $aligned) {
        $improvement = ([double]$tiled.Time_ms - [double]$aligned.Time_ms) / [double]$tiled.Time_ms * 100
        $sign = if ($improvement -gt 0) { "+" } else { "" }
        Write-Host ("  N={0}: {1}{2:F1}% ({3}ms vs {4}ms)" -f `
            $N, $sign, $improvement, $tiled.Time_ms, $aligned.Time_ms) -ForegroundColor Gray
    }
}

Write-Host "`n========== All Tests Complete ==========" -ForegroundColor Cyan
Write-Host "Results directory: $result_dir/" -ForegroundColor White
