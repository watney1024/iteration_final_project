# Gauss-Seidel 2D Quick Test Script (single size, multiple threads)

param(
    [int]$GridSize = 512,
    [int[]]$ThreadCounts = @(1, 2, 4, 8, 10, 16, 20)
)

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Gauss-Seidel 2D Quick Performance Test" -ForegroundColor Cyan
Write-Host "  Grid Size: $GridSize x $GridSize" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Compile
Write-Host "Compiling..." -ForegroundColor Yellow
g++ -O2 -std=c++11 -fopenmp -o test_gs_2d_batch.exe test_gs_2d_batch.cpp gauss_seidel_2d.cpp

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    exit 1
}
Write-Host "Compilation successful!`n" -ForegroundColor Green

$baselineTime = 0

foreach ($threads in $ThreadCounts) {
    Write-Host "Testing $threads threads..." -ForegroundColor Yellow
    
    $output = & .\test_gs_2d_batch.exe $GridSize $threads 2>&1
    
    $timeMatch = $output | Select-String -Pattern "Total time:\s+([\d.]+)\s+ms"
    $iterMatch = $output | Select-String -Pattern "Iterations:\s+(\d+)"
    
    if ($timeMatch) {
        $time = [double]$timeMatch.Matches[0].Groups[1].Value
        
        if ($threads -eq 1) {
            $baselineTime = $time
            Write-Host "  Time: $time ms (baseline)" -ForegroundColor Green
        } else {
            $speedup = [math]::Round($baselineTime / $time, 2)
            $efficiency = [math]::Round($speedup / $threads * 100, 1)
            Write-Host "  Time: $time ms | Speedup: ${speedup}x | Efficiency: ${efficiency}%" -ForegroundColor Green
        }
        
        if ($iterMatch) {
            $iters = $iterMatch.Matches[0].Groups[1].Value
            Write-Host "  Iterations: $iters`n" -ForegroundColor Gray
        }
    } else {
        Write-Host "  Test failed`n" -ForegroundColor Red
    }
}

Write-Host "Test completed!" -ForegroundColor Cyan
