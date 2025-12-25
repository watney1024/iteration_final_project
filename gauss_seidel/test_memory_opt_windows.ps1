# -*- coding: utf-8 -*-
# Memory Optimization Performance Test Script for Windows
# No make required - uses g++ directly

# Set console encoding to UTF-8
$OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
chcp 65001 > $null

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  Memory Optimization Performance Test" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# Change to gauss_seidel directory
Push-Location "gauss_seidel"

# Check if g++ is available
Write-Host "[1/3] Checking compiler..." -ForegroundColor Yellow
$gppExists = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gppExists) {
    Write-Host "ERROR: g++ not found!" -ForegroundColor Red
    Write-Host "Please install MinGW or MSYS2:" -ForegroundColor Yellow
    Write-Host "  - MinGW: https://sourceforge.net/projects/mingw/" -ForegroundColor Cyan
    Write-Host "  - MSYS2: https://www.msys2.org/" -ForegroundColor Cyan
    Pop-Location
    exit 1
}
Write-Host "Compiler found: g++`n" -ForegroundColor Green

# Compile optimized version
Write-Host "[2/3] Compiling optimized version..." -ForegroundColor Yellow
Write-Host "This may take 30-60 seconds..." -ForegroundColor Gray

# Try with AVX2 first
$compileCmd = "g++ -O3 -march=native -fopenmp -mavx2 -mfma -funroll-loops " +
              "gauss_seidel_2d.cpp gauss_seidel_2d_optimized.cpp test_memory_optimization.cpp " +
              "-o test_memory_optimization.exe"

Write-Host "Trying: g++ with AVX2 optimizations..." -ForegroundColor Gray
$result = Invoke-Expression $compileCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "AVX2 not supported, trying without AVX2..." -ForegroundColor Yellow
    $compileCmd = "g++ -O3 -march=native -fopenmp " +
                  "gauss_seidel_2d.cpp gauss_seidel_2d_optimized.cpp test_memory_optimization.cpp " +
                  "-o test_memory_optimization.exe"
    $result = Invoke-Expression $compileCmd 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Compilation failed!" -ForegroundColor Red
        Write-Host "Error output:" -ForegroundColor Red
        Write-Host $result -ForegroundColor Red
        Pop-Location
        exit 1
    }
}

Write-Host "Compilation complete!`n" -ForegroundColor Green

# Check if executable exists
if (-not (Test-Path "test_memory_optimization.exe")) {
    Write-Host "ERROR: Executable not found after compilation!" -ForegroundColor Red
    Pop-Location
    exit 1
}

# Test configuration
$sizes = @(256, 512)  # Reduced for faster testing
$threads = @(4, 8)

Write-Host "[3/3] Starting performance tests...`n" -ForegroundColor Yellow
Write-Host "Note: Testing with N=256,512 and threads=4,8 (reduced for speed)" -ForegroundColor Gray
Write-Host "To test larger sizes, edit the script`n" -ForegroundColor Gray

foreach ($N in $sizes) {
    foreach ($t in $threads) {
        Write-Host "Testing: N=$N, Threads=$t" -ForegroundColor Cyan
        Write-Host "----------------------------------------"
        
        & ".\test_memory_optimization.exe" $N $t
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Test failed for N=$N, threads=$t" -ForegroundColor Red
        }
        
        Write-Host ""
    }
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Key Findings Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "1. Tiling optimization shows best results on large-scale problems" -ForegroundColor White
Write-Host "2. Data restructuring suits very large-scale + high iteration counts" -ForegroundColor White
Write-Host "3. SIMD has limited effect on red-black ordering" -ForegroundColor White
Write-Host "4. Multi-threading + Tiling achieves best performance" -ForegroundColor White
Write-Host "`nDetailed report: MEMORY_OPTIMIZATION_REPORT.md`n" -ForegroundColor Yellow

# Return to original directory
Pop-Location
