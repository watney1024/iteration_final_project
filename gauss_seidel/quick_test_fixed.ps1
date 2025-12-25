# Quick test script for fixed 3D Gauss-Seidel implementation
# Tests 128^3 and 256^3 grids with different thread counts

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Fixed 3D Gauss-Seidel Performance Test" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Test configurations
$gridSizes = @(128, 256)
$threadCounts = @(1, 2, 4, 8)

# Compile if needed
if (-not (Test-Path "test_gs_3d_batch.exe")) {
    Write-Host "Compiling..." -ForegroundColor Yellow
    g++ -O2 -std=c++11 -fopenmp -o test_gs_3d_batch.exe test_gs_3d_batch.cpp gauss_seidel_3d.cpp
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Compilation failed!" -ForegroundColor Red
        exit 1
    }
}

foreach ($gridSize in $gridSizes) {
    Write-Host ""
    Write-Host "Testing Grid: $gridSize x $gridSize x $gridSize" -ForegroundColor Cyan
    Write-Host ("=" * 70) -ForegroundColor Cyan
    Write-Host ""
    
    $results = @{}
    
    foreach ($threads in $threadCounts) {
        Write-Host "  Running with $threads thread(s)..." -ForegroundColor Yellow -NoNewline
        
        $output = & .\test_gs_3d_batch.exe $gridSize $threads 2>&1
        
        # Extract time
        $timeMatch = $output | Select-String -Pattern "Total time:\s+([\d.]+)\s+ms"
        if ($timeMatch) {
            $time = [double]$timeMatch.Matches[0].Groups[1].Value
            $results[$threads] = $time
            Write-Host " $time ms" -ForegroundColor Green
        } else {
            Write-Host " FAILED" -ForegroundColor Red
        }
    }
    
    # Calculate and display speedups
    Write-Host ""
    Write-Host ("=" * 70) -ForegroundColor Cyan
    Write-Host ("线程数`t`t时间(ms)`t`t加速比`t`t效率(%)") -ForegroundColor White
    Write-Host ("-" * 70) -ForegroundColor Cyan
    
    $baseline = $results[1]
    foreach ($threads in $threadCounts) {
        if ($results.ContainsKey($threads)) {
            $time = $results[$threads]
            $speedup = $baseline / $time
            $efficiency = ($speedup / $threads) * 100
            
            $color = "White"
            if ($speedup -gt ($threads * 0.7)) { $color = "Green" }
            elseif ($speedup -gt ($threads * 0.5)) { $color = "Yellow" }
            else { $color = "Red" }
            
            $speedupStr = "{0:F2}x" -f $speedup
            $effStr = "{0:F1}%" -f $efficiency
            
            Write-Host ("{0}`t`t{1,8:F2}`t`t{2}`t`t{3}" -f $threads, $time, $speedupStr, $effStr) -ForegroundColor $color
        }
    }
    Write-Host ""
}

Write-Host ""
Write-Host "Test completed!" -ForegroundColor Green
