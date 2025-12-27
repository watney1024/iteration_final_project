# PowerShell script to gather system information and compiler details

function Get-SystemInfo {
    Write-Host @'
==========================================================
                    SYSTEM INFORMATION
==========================================================
'@

    try {
        $os = Get-WmiObject -Class Win32_OperatingSystem
        $cpu = Get-WmiObject -Class Win32_Processor
        $ram = Get-WmiObject -Class Win32_ComputerSystemProduct
        $computerSystem = Get-WmiObject -Class Win32_ComputerSystem

        Write-Host "Operating System: $($os.Caption)"
        Write-Host "OS Version: $($os.Version)"
        Write-Host "OS Build: $($os.BuildNumber)"
        Write-Host "System Name: $($computerSystem.Name)"
        Write-Host ""
        Write-Host "CPU Information:"
        Write-Host "  Processor: $($cpu.Name)"
        Write-Host "  Cores: $($cpu.NumberOfCores)"
        Write-Host "  Logical Processors: $($cpu.NumberOfLogicalProcessors)"
        Write-Host "  Max Clock Speed: $($cpu.MaxClockSpeed) MHz"
        Write-Host ""
        
        $totalMemory = $computerSystem.TotalPhysicalMemory / 1GB
        Write-Host "Memory: $([Math]::Round($totalMemory, 2)) GB"
        Write-Host ""
    } catch {
        Write-Host "Error retrieving system information: $_"
    }
}

function Get-CompilerInfo {
    Write-Host @'
==========================================================
                   COMPILER INFORMATION
==========================================================
'@

    # Check for GCC
    Write-Host "GCC (GNU Compiler Collection):"
    try {
        $gccVersion = & gcc --version 2>$null
        if ($LASTEXITCODE -eq 0) {
            $gccVersionLine = $gccVersion | Select-Object -First 1
            Write-Host "  Status: INSTALLED"
            Write-Host "  Version: $gccVersionLine"
        } else {
            Write-Host "  Status: NOT INSTALLED"
        }
    } catch {
        Write-Host "  Status: NOT INSTALLED"
    }
    Write-Host ""

    # Check for Clang
    Write-Host "Clang (LLVM C++ Compiler):"
    try {
        $clangVersion = & clang --version 2>$null
        if ($LASTEXITCODE -eq 0) {
            $clangVersionLine = $clangVersion | Select-Object -First 1
            Write-Host "  Status: INSTALLED"
            Write-Host "  Version: $clangVersionLine"
        } else {
            Write-Host "  Status: NOT INSTALLED"
        }
    } catch {
        Write-Host "  Status: NOT INSTALLED"
    }
    Write-Host ""

    # Check for MSVC
    Write-Host "MSVC (Microsoft Visual C++ Compiler):"
    try {
        $msvcPath = Get-Command cl.exe -ErrorAction SilentlyContinue
        if ($msvcPath) {
            $msvcVersion = & cl.exe 2>&1 | Select-Object -First 1
            Write-Host "  Status: INSTALLED"
            Write-Host "  Path: $($msvcPath.Source)"
            Write-Host "  Version: $msvcVersion"
        } else {
            Write-Host "  Status: NOT INSTALLED"
        }
    } catch {
        Write-Host "  Status: NOT INSTALLED"
    }
    Write-Host ""
}

function Get-OpenMPInfo {
    Write-Host @'
==========================================================
                    OPENMP INFORMATION
==========================================================
'@

    Write-Host "OpenMP Detection:"
    Write-Host ""

    # Check for OpenMP support in GCC
    Write-Host "GCC OpenMP Support:"
    try {
        $testCode = @'
#include <omp.h>
#include <stdio.h>

int main() {
    #pragma omp parallel
    {
        printf("Max threads: %d\n", omp_get_max_threads());
    }
    return 0;
}
'@
        
        $testFile = [System.IO.Path]::Combine($env:TEMP, "test_openmp.cpp")
        $exeFile = [System.IO.Path]::Combine($env:TEMP, "test_openmp.exe")
        
        Set-Content -Path $testFile -Value $testCode -Force
        
        & gcc -fopenmp $testFile -o $exeFile 2>$null
        
        if ($LASTEXITCODE -eq 0 -and (Test-Path $exeFile)) {
            Write-Host "  Status: SUPPORTED"
            $output = & $exeFile 2>$null
            Write-Host "  Result: $output"
            Remove-Item $exeFile -Force -ErrorAction SilentlyContinue
        } else {
            Write-Host "  Status: NOT SUPPORTED or GCC NOT INSTALLED"
        }
        
        Remove-Item $testFile -Force -ErrorAction SilentlyContinue
    } catch {
        Write-Host "  Status: ERROR DURING DETECTION"
        Write-Host "  Details: $_"
    }
    
    Write-Host ""
    
    # Check for OpenMP in MSVC
    Write-Host "MSVC OpenMP Support:"
    try {
        $testCode = @'
#include <omp.h>
#include <stdio.h>

int main() {
    #pragma omp parallel
    {
        printf("Max threads: %d\n", omp_get_max_threads());
    }
    return 0;
}
'@
        
        $testFile = [System.IO.Path]::Combine($env:TEMP, "test_openmp_msvc.cpp")
        $exeFile = [System.IO.Path]::Combine($env:TEMP, "test_openmp_msvc.exe")
        
        Set-Content -Path $testFile -Value $testCode -Force
        
        & cl.exe /openmp $testFile /Fe$exeFile 2>$null
        
        if ($LASTEXITCODE -eq 0 -and (Test-Path $exeFile)) {
            Write-Host "  Status: SUPPORTED"
            $output = & $exeFile 2>$null
            Write-Host "  Result: $output"
            Remove-Item $exeFile -Force -ErrorAction SilentlyContinue
        } else {
            Write-Host "  Status: NOT SUPPORTED or MSVC NOT INSTALLED"
        }
        
        Remove-Item $testFile -Force -ErrorAction SilentlyContinue
    } catch {
        Write-Host "  Status: ERROR DURING DETECTION"
        Write-Host "  Details: $_"
    }
    
    Write-Host ""
    
    # Check system thread count
    Write-Host "System Thread Information:"
    try {
        $logicalProcessors = (Get-WmiObject -Class Win32_Processor).NumberOfLogicalProcessors
        Write-Host "  Logical Processors: $logicalProcessors"
        Write-Host "  Available for OpenMP: Up to $logicalProcessors threads"
    } catch {
        Write-Host "  Error retrieving thread information: $_"
    }
    
    Write-Host ""
}

function Main {
    Get-SystemInfo
    Get-CompilerInfo
    Get-OpenMPInfo
    
    Write-Host @'
==========================================================
                    END OF REPORT
==========================================================
'@
}

Main


(base) PS D:\Code\Python\iteration_final_project> powershell -ExecutionPolicy ByPass -File ".\latex\get_system_info.ps1"
==========================================================
                    SYSTEM INFORMATION
==========================================================
Operating System: Microsoft Windows 11 家庭中文版
OS Version: 10.0.26200
OS Build: 26200
System Name: LAPTOP-QG42EDTI

CPU Information:
  Processor: AMD Ryzen AI 9 H 365 w/ Radeon 880M
  Cores: 10
  Logical Processors: 20
  Max Clock Speed: 2000 MHz

Memory: 27.79 GB

==========================================================
                   COMPILER INFORMATION
==========================================================
GCC (GNU Compiler Collection):
  Status: INSTALLED
  Version: gcc.exe (x86_64-win32-seh-rev0, Built by MinGW-Builds project) 15.2.0   

Clang (LLVM C++ Compiler):
  Status: INSTALLED
  Version: (built by Brecht Sanders, r5) clang version 17.0.6

MSVC (Microsoft Visual C++ Compiler):
  Status: NOT INSTALLED

==========================================================
                    OPENMP INFORMATION
==========================================================
OpenMP Detection:

GCC OpenMP Support:
  Status: NOT SUPPORTED or GCC NOT INSTALLED

MSVC OpenMP Support:
  Status: ERROR DURING DETECTION
  Details: 无法将“cl.exe”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。请检查 名称的拼写，如果包括路径，请确保路径正确，然后再试一次。

System Thread Information:
  Logical Processors: 20
  Available for OpenMP: Up to 20 threads

==========================================================
                    END OF REPORT
==========================================================