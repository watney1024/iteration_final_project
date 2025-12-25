# 对比修复前后的Tiled版本性能
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  3D Gauss-Seidel: 原始 vs 优化Tiling 性能对比" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Test configurations
$gridSizes = @(128, 256)
$threadCounts = @(1, 2, 4, 8)

# Compile
Write-Host "编译程序..." -ForegroundColor Yellow
g++ -O3 -march=native -fopenmp gauss_seidel_3d.cpp gauss_seidel_3d_tiled.cpp test_tiled_performance_3d.cpp -o test_tiled_3d.exe
if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败!" -ForegroundColor Red
    exit 1
}
Write-Host "编译成功!`n" -ForegroundColor Green

foreach ($gridSize in $gridSizes) {
    Write-Host ""
    Write-Host "测试网格: $gridSize x $gridSize x $gridSize" -ForegroundColor Cyan
    Write-Host ("=" * 90) -ForegroundColor Cyan
    Write-Host ""
    
    Write-Host ("{0,-10} {1,12} {2,12} {3,12} {4,12} {5,12}" -f `
        "线程数", "原始(ms)", "优化(ms)", "加速比", "提升", "效率(%)") -ForegroundColor White
    Write-Host ("{0,-10} {1,12} {2,12} {3,12} {4,12} {5,12}" -f `
        "------", "---------", "---------", "-------", "-------", "--------") -ForegroundColor Cyan
    
    foreach ($threads in $threadCounts) {
        $output = & .\test_tiled_3d.exe $gridSize $threads 2>&1 | Out-String
        
        # Parse results
        $time_orig = 0
        $time_tiled = 0
        
        if ($output -match "Original\s+\|\s+\d+\s+\|\s+([\d.]+)") {
            $time_orig = [double]$matches[1]
        }
        
        if ($output -match "2-Layer Tiling\s+\|\s+\d+\s+\|\s+([\d.]+)") {
            $time_tiled = [double]$matches[1]
        }
        
        if ($time_orig -gt 0 -and $time_tiled -gt 0) {
            $speedup = $time_orig / $time_tiled
            $improvement = (($time_orig - $time_tiled) / $time_orig) * 100
            $efficiency = ($speedup / $threads) * 100
            
            $color = "White"
            if ($speedup -gt 1.2) { $color = "Green" }
            elseif ($speedup -gt 1.0) { $color = "Yellow" }
            else { $color = "Red" }
            
            Write-Host ("{0,-10} {1,12:F2} {2,12:F2} {3,12:F2}x {4,11:F1}% {5,11:F1}%" -f `
                $threads, $time_orig, $time_tiled, $speedup, $improvement, $efficiency) -ForegroundColor $color
        } else {
            Write-Host "  $threads 线程: 测试失败" -ForegroundColor Red
        }
    }
    Write-Host ""
}

Write-Host ""
Write-Host "测试完成!" -ForegroundColor Green
Write-Host ""
Write-Host "关键优化点:" -ForegroundColor Yellow
Write-Host "  1. 消除条件分支 - k循环直接跳2步" -ForegroundColor White
Write-Host "  2. 动态调度 - 改善负载均衡" -ForegroundColor White
Write-Host "  3. 增大tile_size - 减少同步开销" -ForegroundColor White
Write-Host "  4. 提取常量 - 避免重复计算" -ForegroundColor White
Write-Host "  5. 减少残差检查 - 降低同步频率" -ForegroundColor White
