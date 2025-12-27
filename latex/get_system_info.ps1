# 系统信息查询脚本
# 用于收集论文所需的测试环境信息

Write-Host "==================================" -ForegroundColor Cyan
Write-Host "系统信息查询" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
Write-Host ""

# 1. 处理器信息
Write-Host "【处理器信息】" -ForegroundColor Yellow
$cpu = Get-WmiObject Win32_Processor
Write-Host "型号: $($cpu.Name.Trim())"
Write-Host "核心数: $($cpu.NumberOfCores)"
Write-Host "逻辑处理器数(线程): $($cpu.NumberOfLogicalProcessors)"
Write-Host "基础频率: $([math]::Round($cpu.MaxClockSpeed/1000, 2)) GHz"
Write-Host ""

# 2. 内存信息
Write-Host "【内存信息】" -ForegroundColor Yellow
$memory = Get-WmiObject Win32_PhysicalMemory
$totalMemoryGB = ($memory | Measure-Object -Property Capacity -Sum).Sum / 1GB
$memorySpeed = ($memory | Select-Object -First 1).Speed
Write-Host "总内存: $([math]::Round($totalMemoryGB)) GB"
Write-Host "内存频率: DDR4-$memorySpeed"
Write-Host ""

# 3. 操作系统信息
Write-Host "【操作系统】" -ForegroundColor Yellow
$os = Get-WmiObject Win32_OperatingSystem
Write-Host "系统: $($os.Caption)"
Write-Host "版本: $($os.Version)"
Write-Host "架构: $($os.OSArchitecture)"
Write-Host ""

# 4. 编译器信息 (检查常见的C++编译器)
Write-Host "【编译器信息】" -ForegroundColor Yellow

# 检查 GCC
try {
    $gccVersion = & gcc --version 2>&1 | Select-Object -First 1
    if ($gccVersion) {
        Write-Host "GCC: $gccVersion"
    }
} catch {
    Write-Host "GCC: 未安装或不在PATH中" -ForegroundColor Gray
}

# 检查 Clang
try {
    $clangVersion = & clang --version 2>&1 | Select-Object -First 1
    if ($clangVersion) {
        Write-Host "Clang: $clangVersion"
    }
} catch {
    Write-Host "Clang: 未安装或不在PATH中" -ForegroundColor Gray
}

# 检查 MSVC
try {
    $clVersion = & cl 2>&1 | Select-String "Version"
    if ($clVersion) {
        Write-Host "MSVC: $clVersion"
    }
} catch {
    Write-Host "MSVC: 未安装或不在PATH中" -ForegroundColor Gray
}

Write-Host ""

# 5. OpenMP信息
Write-Host "【OpenMP信息】" -ForegroundColor Yellow

# 创建临时测试程序
$tempDir = [System.IO.Path]::GetTempPath()
$testFile = Join-Path $tempDir "omp_test.cpp"
$exeFile = Join-Path $tempDir "omp_test.exe"

$ompTestCode = @"
#include <iostream>
#include <omp.h>

int main() {
    #ifdef _OPENMP
        std::cout << "OpenMP Version: " << _OPENMP << std::endl;
        std::cout << "Max Threads: " << omp_get_max_threads() << std::endl;
    #else
        std::cout << "OpenMP not supported" << std::endl;
    #endif
    return 0;
}
"@

Set-Content -Path $testFile -Value $ompTestCode

# 尝试使用 g++ 编译并运行
try {
    $null = & g++ -fopenmp $testFile -o $exeFile 2>&1
    if (Test-Path $exeFile) {
        $ompInfo = & $exeFile
        Write-Host $ompInfo
        Remove-Item $exeFile -ErrorAction SilentlyContinue
    } else {
        Write-Host "无法检测OpenMP版本（编译失败）" -ForegroundColor Gray
    }
} catch {
    Write-Host "无法检测OpenMP版本（GCC未安装或不支持OpenMP）" -ForegroundColor Gray
}

Remove-Item $testFile -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "==================================" -ForegroundColor Cyan
Write-Host "查询完成" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
