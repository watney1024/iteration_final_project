# Gauss-Seidel 3D Quick Test Script (single size, multiple threads)

param(
    [int]$GridSize = 256,
    [int[]]$ThreadCounts = @(1, 2, 4, 8, 10, 16, 20)
)

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Gauss-Seidel 3D Quick Performance Test" -ForegroundColor Cyan
Write-Host "  Grid Size: $GridSize x $GridSize x $GridSize" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Compile
Write-Host "Compiling..." -ForegroundColor Yellow
g++ -O2 -std=c++11 -fopenmp -o test_gs_3d_batch.exe test_gs_3d_batch.cpp gauss_seidel_3d.cpp

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    exit 1
}
Write-Host "Compilation successful!`n" -ForegroundColor Green

$baselineTime = 0

foreach ($threads in $ThreadCounts) {
    Write-Host "Testing $threads threads..." -ForegroundColor Yellow
    
    $output = & .\test_gs_3d_batch.exe $GridSize $threads 2>&1
    
    $timeMatch = $output | Select-String -Pattern "Total time:\s+([\d.]+)\s+s"
    $iterMatch = $output | Select-String -Pattern "Iterations:\s+(\d+)"
    $gflopsMatch = $output | Select-String -Pattern "Performance:\s+([\d.]+)\s+GFLOPS"
    
    if ($timeMatch) {
        $time = [double]$timeMatch.Matches[0].Groups[1].Value
        
        if ($threads -eq 1) {
            $baselineTime = $time
            Write-Host "  Time: $time s (baseline)" -ForegroundColor Green
        } else {
            $speedup = [math]::Round($baselineTime / $time, 2)
            $efficiency = [math]::Round($speedup / $threads * 100, 1)
            Write-Host "  Time: $time s | Speedup: ${speedup}x | Efficiency: ${efficiency}%" -ForegroundColor Green
        }
        
        if ($iterMatch) {
            $iters = $iterMatch.Matches[0].Groups[1].Value
            Write-Host "  Iterations: $iters" -ForegroundColor Gray
        }
        
        if ($gflopsMatch) {
            $gflops = $gflopsMatch.Matches[0].Groups[1].Value
            Write-Host "  Performance: $gflops GFLOPS`n" -ForegroundColor Gray
        }
    } else {
        Write-Host "  Test failed or timeout`n" -ForegroundColor Red
    }
}

Write-Host "Test completed!" -ForegroundColor Cyan
