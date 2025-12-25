# 快速测试2D小规模性能修复
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  2D Gauss-Seidel小规模性能测试" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# 编译
Write-Host "编译..." -ForegroundColor Yellow
g++ -O2 -fopenmp -o test_tiled_2d.exe test_tiled_performance.cpp gauss_seidel_2d.cpp gauss_seidel_2d_tiled.cpp
if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败!" -ForegroundColor Red
    exit 1
}
Write-Host "编译成功!`n" -ForegroundColor Green

$gridSizes = @(64, 128, 256, 512)
$threadCounts = @(1, 2, 4, 8)

foreach ($N in $gridSizes) {
    Write-Host "测试 ${N}x${N}" -ForegroundColor Cyan
    Write-Host ("{0,-8} {1,12} {2,12} {3,12}" -f "线程", "原始(ms)", "Tiled(ms)", "加速比") -ForegroundColor White
    Write-Host ("{0,-8} {1,12} {2,12} {3,12}" -f "----", "-------", "-------", "-----") -ForegroundColor Cyan
    
    $times_orig = @{}
    $times_tiled = @{}
    
    foreach ($t in $threadCounts) {
        $output = & .\test_tiled_2d.exe 2d $N $t 2>&1 | Out-String
        
        if ($output -match "Original\s+\|\s+\d+\s+\|\s+([\d.]+)") {
            $times_orig[$t] = [double]$matches[1]
        }
        if ($output -match "2-Layer Tiling\s+\|\s+\d+\s+\|\s+([\d.]+)") {
            $times_tiled[$t] = [double]$matches[1]
        }
        
        if ($times_orig.ContainsKey($t) -and $times_tiled.ContainsKey($t)) {
            $orig = $times_orig[$t]
            $tiled = $times_tiled[$t]
            $speedup = $orig / $tiled
            
            $color = "White"
            if ($speedup -gt 1.2) { $color = "Green" }
            elseif ($speedup -gt 1.0) { $color = "Yellow" }
            else { $color = "Red" }
            
            Write-Host ("{0,-8} {1,12:F2} {2,12:F2} {3,12:F2}x" -f $t, $orig, $tiled, $speedup) -ForegroundColor $color
        }
    }
    
    # 并行扩展性
    Write-Host "`n并行扩展性:" -ForegroundColor Yellow
    Write-Host ("{0,-8} {1,15} {2,15}" -f "线程", "原始加速比", "Tiled加速比") -ForegroundColor White
    Write-Host ("{0,-8} {1,15} {2,15}" -f "----", "----------", "-----------") -ForegroundColor Cyan
    
    $base_orig = $times_orig[1]
    $base_tiled = $times_tiled[1]
    foreach ($t in $threadCounts) {
        if ($times_orig.ContainsKey($t) -and $times_tiled.ContainsKey($t)) {
            $s_orig = $base_orig / $times_orig[$t]
            $s_tiled = $base_tiled / $times_tiled[$t]
            
            $color_orig = if ($s_orig -gt ($t * 0.7)) { "Green" } elseif ($s_orig -gt ($t * 0.5)) { "Yellow" } else { "Red" }
            $color_tiled = if ($s_tiled -gt ($t * 0.7)) { "Green" } elseif ($s_tiled -gt ($t * 0.5)) { "Yellow" } else { "Red" }
            
            Write-Host ("{0,-8} " -f $t) -NoNewline
            Write-Host ("{0,15:F2}x" -f $s_orig) -ForegroundColor $color_orig -NoNewline
            Write-Host "  " -NoNewline
            Write-Host ("{0,15:F2}x" -f $s_tiled) -ForegroundColor $color_tiled
        }
    }
    Write-Host ""
}

Write-Host "`n测试完成!" -ForegroundColor Green
