# Gauss-Seidel 3D Batch Testing Script
# Test grid sizes: 128^3, 256^3, 512^3

# Switch to script directory
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Gauss-Seidel 3D Batch Performance Test" -ForegroundColor Cyan
Write-Host "  Grid Sizes: 128^3, 256^3, 512^3" -ForegroundColor Cyan
Write-Host "  Thread Counts: 1, 2, 4, 8, 10, 16, 20" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Create results directory
$resultDir = ".\results_3d"
if (-not (Test-Path $resultDir)) {
    New-Item -ItemType Directory -Path $resultDir | Out-Null
    Write-Host "Created results directory: $resultDir" -ForegroundColor Green
}

# Define test configuration
$gridSizes = @(128, 256, 512)
$threadCounts = @(1, 2, 4, 8, 10, 16, 20)
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$summaryFile = "$resultDir\summary_3d_$timestamp.txt"

# Write summary file header
"Gauss-Seidel 3D Performance Test Summary" | Out-File -FilePath $summaryFile -Encoding UTF8
"Test Time: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"=" * 80 | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"" | Out-File -FilePath $summaryFile -Append -Encoding UTF8

# Compile source code
Write-Host "Compiling gauss_seidel_3d test program..." -ForegroundColor Yellow
$compileCmd = "g++ -O2 -std=c++11 -fopenmp -o test_gs_3d_batch.exe test_gs_3d_batch.cpp gauss_seidel_3d.cpp"
Invoke-Expression $compileCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    exit 1
}
Write-Host "Compilation successful!" -ForegroundColor Green
Write-Host ""

# Test each grid size
foreach ($gridSize in $gridSizes) {
    $totalPoints = [math]::Pow($gridSize, 3)
    $memoryGB = [math]::Round($totalPoints * 8 * 3 / 1GB, 2)
    
    Write-Host "============================================================" -ForegroundColor Cyan
    Write-Host "  Testing grid size: $gridSize x $gridSize x $gridSize" -ForegroundColor Cyan
    Write-Host "  Total points: $totalPoints" -ForegroundColor Cyan
    Write-Host "  Estimated memory: ~$memoryGB GB" -ForegroundColor Cyan
    Write-Host "============================================================" -ForegroundColor Cyan
    
    # Write to summary file
    "" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
    "Grid Size: $gridSize x $gridSize x $gridSize (Points: $totalPoints, Memory: ~$memoryGB GB)" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
    "-" * 80 | Out-File -FilePath $summaryFile -Append -Encoding UTF8
    
    # Test different thread counts
    foreach ($threads in $threadCounts) {
        Write-Host "  Running test: $threads threads..." -ForegroundColor Yellow
        
        $outputFile = "$resultDir\gs3d_${gridSize}_t${threads}_$timestamp.txt"
        
        # Run test and capture output
        try {
            $startTime = Get-Date
            $output = & .\test_gs_3d_batch.exe $gridSize $threads 2>&1
            $endTime = Get-Date
            $elapsed = ($endTime - $startTime).TotalSeconds
            
            # Save complete output
            $output | Out-File -FilePath $outputFile -Encoding UTF8
            
            # Extract key information
            $timeMatch = $output | Select-String -Pattern "Total time:\s+([\d.]+)\s+ms"
            $iterMatch = $output | Select-String -Pattern "Iterations:\s+(\d+)"
            $gflopsMatch = $output | Select-String -Pattern "Performance:\s+([\d.]+)\s+GFLOPS"
            
            if ($timeMatch) {
                $time = $timeMatch.Matches[0].Groups[1].Value
                Write-Host "    Time: $time ms (Elapsed: $([math]::Round($elapsed, 1)) s)" -ForegroundColor Green
                
                if ($iterMatch) {
                    $iters = $iterMatch.Matches[0].Groups[1].Value
                    $perIter = [math]::Round([double]$time / [int]$iters, 3)
                    Write-Host "    Iterations: $iters, Avg: $perIter ms/iter" -ForegroundColor Green
                    
                    # Write to summary
                    $summary = "  [$threads threads] Time: $time ms | Iters: $iters | Avg: $perIter ms/iter"
                    if ($gflopsMatch) {
                        $gflops = $gflopsMatch.Matches[0].Groups[1].Value
                        $summary += " | GFLOPS: $gflops"
                        Write-Host "    Performance: $gflops GFLOPS" -ForegroundColor Green
                    }
                    $summary | Out-File -FilePath $summaryFile -Append -Encoding UTF8
                }
            } else {
                Write-Host "    Cannot parse output (not converged or timeout)" -ForegroundColor Red
                "  [$threads threads] Test failed, timeout or not converged (Elapsed: $([math]::Round($elapsed, 1)) s)" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
            }
        } catch {
            Write-Host "    Test error: $_" -ForegroundColor Red
            "  [$threads threads] Test error: $_" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
        }
        
        Start-Sleep -Milliseconds 1000
    }
    
    Write-Host ""
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  All tests completed!" -ForegroundColor Green
Write-Host "  Results saved in: $resultDir" -ForegroundColor Green
Write-Host "  Summary file: $summaryFile" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Note: 512^3 test may take several minutes to complete" -ForegroundColor Yellow
Write-Host "      May crash if insufficient memory" -ForegroundColor Yellow
