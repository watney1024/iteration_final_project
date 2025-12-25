# Quick test script for Tiled 3D Gauss-Seidel implementation
# Tests 128^3 and 256^3 grids with different thread counts

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  3D Gauss-Seidel: Original vs Tiled Performance Test" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Test configurations
$gridSizes = @(128, 256)
$threadCounts = @(1, 2, 4, 8)

# Compile if needed
if (-not (Test-Path "test_tiled_3d.exe")) {
    Write-Host "Compiling..." -ForegroundColor Yellow
    g++ -O2 -fopenmp -o test_tiled_3d.exe test_tiled_performance_3d.cpp gauss_seidel_3d.cpp gauss_seidel_3d_tiled.cpp
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Compilation failed!" -ForegroundColor Red
        exit 1
    }
    Write-Host "Compilation successful!" -ForegroundColor Green
}

foreach ($gridSize in $gridSizes) {
    Write-Host ""
    Write-Host "Testing Grid: $gridSize x $gridSize x $gridSize" -ForegroundColor Cyan
    Write-Host ("=" * 95) -ForegroundColor Cyan
    Write-Host ""
    
    $origResults = @{}
    $tiledResults = @{}
    
    foreach ($threads in $threadCounts) {
        Write-Host "  Running with $threads thread(s)..." -ForegroundColor Yellow -NoNewline
        
        $output = & .\test_tiled_3d.exe $gridSize $threads 2>&1 | Out-String
        
        # Extract times from output
        # Original format: "Original          | 100      | 711.50     | 7.92e-01 | 1.00x"
        if ($output -match "Original\s+\|\s+\d+\s+\|\s+([\d.]+)") {
            $origResults[$threads] = [double]$matches[1]
        }
        
        # Tiled format: "2-Layer Tiling    | 100      | 438.22     | 7.92e-01 | 1.62x"
        if ($output -match "2-Layer Tiling\s+\|\s+\d+\s+\|\s+([\d.]+)") {
            $tiledResults[$threads] = [double]$matches[1]
        }
        
        if ($origResults.ContainsKey($threads) -and $tiledResults.ContainsKey($threads)) {
            $orig = $origResults[$threads]
            $tiled = $tiledResults[$threads]
            Write-Host " Original: $orig ms, Tiled: $tiled ms" -ForegroundColor Green
        } else {
            Write-Host " FAILED" -ForegroundColor Red
        }
    }
    
    # Display results table
    Write-Host ""
    Write-Host ("=" * 95) -ForegroundColor Cyan
    Write-Host ("{0,-8} {1,12} {2,12} {3,12} {4,12} {5,12}" -f "线程数", "原始(ms)", "Tiled(ms)", "加速比", "提升(%)", "效率(%)") -ForegroundColor White
    Write-Host ("{0,-8} {1,12} {2,12} {3,12} {4,12} {5,12}" -f "------", "---------", "---------", "-------", "-------", "--------") -ForegroundColor Cyan
    
    $baseline_orig = $origResults[1]
    $baseline_tiled = $tiledResults[1]
    
    foreach ($threads in $threadCounts) {
        if ($origResults.ContainsKey($threads) -and $tiledResults.ContainsKey($threads)) {
            $orig = $origResults[$threads]
            $tiled = $tiledResults[$threads]
            
            # 计算Tiled相对于Original的加速比
            $speedup = $orig / $tiled
            $improvement = (($orig - $tiled) / $orig) * 100
            
            # 计算Tiled版本的并行效率（相对于Tiled的单线程版本）
            $parallel_speedup = $baseline_tiled / $tiled
            $efficiency = ($parallel_speedup / $threads) * 100
            
            $color = "White"
            if ($speedup -gt 1.3) { $color = "Green" }
            elseif ($speedup -gt 1.1) { $color = "Yellow" }
            else { $color = "Red" }
            
            Write-Host ("{0,-8} {1,12:F2} {2,12:F2} {3,12:F2}x {4,11:F1}% {5,11:F1}%" -f `
                $threads, $orig, $tiled, $speedup, $improvement, $efficiency) -ForegroundColor $color
        }
    }
    
    # Display parallel scaling for both versions
    Write-Host ""
    Write-Host "并行扩展性分析:" -ForegroundColor Yellow
    Write-Host ("{0,-8} {1,20} {2,20}" -f "线程数", "原始加速比", "Tiled加速比") -ForegroundColor White
    Write-Host ("{0,-8} {1,20} {2,20}" -f "------", "---------------", "---------------") -ForegroundColor Cyan
    
    foreach ($threads in $threadCounts) {
        if ($origResults.ContainsKey($threads) -and $tiledResults.ContainsKey($threads)) {
            $orig_speedup = $baseline_orig / $origResults[$threads]
            $tiled_speedup = $baseline_tiled / $tiledResults[$threads]
            
            $orig_color = "White"
            if ($orig_speedup -gt ($threads * 0.7)) { $orig_color = "Green" }
            elseif ($orig_speedup -gt ($threads * 0.5)) { $orig_color = "Yellow" }
            else { $orig_color = "Red" }
            
            $tiled_color = "White"
            if ($tiled_speedup -gt ($threads * 0.7)) { $tiled_color = "Green" }
            elseif ($tiled_speedup -gt ($threads * 0.5)) { $tiled_color = "Yellow" }
            else { $tiled_color = "Red" }
            
            Write-Host ("{0,-8} " -f $threads) -NoNewline
            Write-Host ("{0,20:F2}x" -f $orig_speedup) -ForegroundColor $orig_color -NoNewline
            Write-Host " " -NoNewline
            Write-Host ("{0,20:F2}x" -f $tiled_speedup) -ForegroundColor $tiled_color
        }
    }
    Write-Host ""
}

Write-Host ""
Write-Host "Test completed!" -ForegroundColor Green
Write-Host ""
Write-Host "关键差异:" -ForegroundColor Yellow
Write-Host "  ? 原始版本: 已优化 (消除分支 + 动态调度 + 大tile)" -ForegroundColor White
Write-Host "  ? Tiled版本: 进一步优化 (更好的缓存局部性)" -ForegroundColor White
Write-Host "  ? 加速比 = Tiled相对于Original的性能提升" -ForegroundColor White
Write-Host "  ? 效率 = Tiled版本的并行效率" -ForegroundColor White
