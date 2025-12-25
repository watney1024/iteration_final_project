# 批量测试三对角矩阵求解器脚本 - 内存数据生成版本
# 测试串行版本和两个并行版本（Brugnano 和 Recursive Doubling）
# 不同数据大小和线程数

# 切换到脚本所在目录
Set-Location -Path $PSScriptRoot

# 设置编码为 UTF-8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 创建结果目录
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultDir = "batch_results_memtest_$timestamp"
New-Item -ItemType Directory -Force -Path $resultDir | Out-Null

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "批量测试三对角矩阵求解器（内存版本）" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "结果目录: $resultDir" -ForegroundColor Green
Write-Host ""

# 编译所有程序
Write-Host "正在编译程序..." -ForegroundColor Yellow
try {
    # 编译串行版本
    Write-Host "  [1/3] 编译 sequential_solver_memtest..." -ForegroundColor Gray
    g++ -O2 -std=c++11 -Wall -o sequential_solver_memtest.exe sequential_solver_memtest.cpp
    if ($LASTEXITCODE -ne 0) { throw "sequential_solver_memtest 编译失败" }
    
    # 编译 Brugnano 版本
    Write-Host "  [2/3] 编译 openmp_brugnano_memtest..." -ForegroundColor Gray
    g++ -fopenmp -O2 -std=c++11 -Wall -o openmp_brugnano_memtest.exe openmp_brugnano_memtest.cpp
    if ($LASTEXITCODE -ne 0) { throw "openmp_brugnano_memtest 编译失败" }
    
    # 编译 Recursive Doubling 版本
    Write-Host "  [3/3] 编译 openmp_recursive_doubling_memtest..." -ForegroundColor Gray
    g++ -fopenmp -O2 -std=c++11 -Wall -o openmp_recursive_doubling_memtest.exe openmp_recursive_doubling_memtest.cpp
    if ($LASTEXITCODE -ne 0) { throw "openmp_recursive_doubling_memtest 编译失败" }
    
    Write-Host "? 所有程序编译成功!" -ForegroundColor Green
    Write-Host ""
} catch {
    Write-Host "? 编译失败: $_" -ForegroundColor Red
    exit 1
}

# 定义测试数据规模
$dataSizes = @(
    @{Name="8k"; Size=8192},
    @{Name="16k"; Size=16384},
    @{Name="128k"; Size=131072},
    @{Name="1M"; Size=1048576},
    @{Name="4M"; Size=4194304}
)

# 定义线程数配置（不包括串行）
$threadCounts = @(1, 2, 4, 8, 10, 16, 20)

# 测试程序列表
$programs = @(
    @{Name="Sequential"; Exe="sequential_solver_memtest.exe"; IsSerial=$true},
    @{Name="Brugnano"; Exe="openmp_brugnano_memtest.exe"; IsSerial=$false},
    @{Name="RecursiveDoubling"; Exe="openmp_recursive_doubling_memtest.exe"; IsSerial=$false}
)

# 函数：从程序输出中提取时间（秒）
function Extract-Time {
    param([string]$output)
    
    # 匹配 "Solve time: X.XXXXXX seconds" 格式
    if ($output -match "Solve time:\s+([\d.]+)\s+seconds") {
        return [double]$matches[1]
    }
    return $null
}

# 函数：运行单个测试
function Run-Test {
    param(
        [string]$exe,
        [int]$dataSize,
        [int]$threads = 1
    )
    
    # 运行程序并捕获输出（传递数据规模和线程数作为参数）
    if ($threads -eq 1 -and $exe -like "*sequential*") {
        # 串行版本只需要数据规模参数
        $output = & ".\$exe" $dataSize 2>&1 | Out-String
    } else {
        # 并行版本需要数据规模和线程数参数
        $output = & ".\$exe" $dataSize $threads 2>&1 | Out-String
    }
    
    # 提取时间
    $time = Extract-Time $output
    
    return @{
        Time = $time
        Output = $output
    }
}

# 主测试循环
Write-Host "开始批量测试..." -ForegroundColor Yellow
Write-Host ""

$totalTests = $dataSizes.Count * (1 + 2 * $threadCounts.Count)
$currentTest = 0

# 汇总结果数组
$summaryResults = @()

foreach ($dataConfig in $dataSizes) {
    $dataName = $dataConfig.Name
    $dataSize = $dataConfig.Size
    
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
    Write-Host "测试数据: $dataName (N=$dataSize)" -ForegroundColor Cyan
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
    
    # 创建数据特定的结果文件
    $dataResultFile = Join-Path $resultDir "${dataName}_results.txt"
    $csvResultFile = Join-Path $resultDir "${dataName}_results.csv"
    
    # 初始化CSV文件
    "Program,Threads,Time(ms),Speedup" | Out-File -FilePath $csvResultFile -Encoding UTF8
    
    # 先运行串行版本作为基准
    Write-Host "`n[1/3] 测试 Sequential (串行基准)..." -ForegroundColor Green
    $currentTest++
    $progress = [math]::Round(($currentTest / $totalTests) * 100, 1)
    Write-Host "    进度: $currentTest/$totalTests ($progress%)" -ForegroundColor Gray
    
    $serialResult = Run-Test -exe "sequential_solver_memtest.exe" -dataSize $dataSize
    $serialTime = $serialResult.Time
    
    if ($serialTime -eq $null) {
        Write-Host "    ? 串行版本运行失败" -ForegroundColor Red
        Write-Host "    输出: $($serialResult.Output)" -ForegroundColor Red
        continue
    }
    
    $serialTimeMs = [math]::Round($serialTime * 1000, 2)
    Write-Host "    ? 串行时间: $serialTimeMs ms" -ForegroundColor White
    
    # 写入结果文件
    "=" * 60 | Out-File -FilePath $dataResultFile -Encoding UTF8
    "数据集: $dataName (N=$dataSize)" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    "=" * 60 | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    "" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    "串行版本 (Sequential)" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    "-" * 60 | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    "时间: $serialTimeMs ms" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    "" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    
    # CSV记录
    "Sequential,1,$serialTimeMs,1.00" | Out-File -FilePath $csvResultFile -Append -Encoding UTF8
    
    # 汇总结果
    $summaryResults += @{
        Data = $dataName
        Size = $dataSize
        Program = "Sequential"
        Threads = 1
        TimeMs = $serialTimeMs
        Speedup = 1.00
    }
    
    # 测试并行版本
    $programIndex = 2
    foreach ($program in $programs | Where-Object { -not $_.IsSerial }) {
        Write-Host "`n[$programIndex/3] 测试 $($program.Name)..." -ForegroundColor Green
        $programIndex++
        
        "并行版本: $($program.Name)" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
        "-" * 60 | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
        
        foreach ($threads in $threadCounts) {
            $currentTest++
            $progress = [math]::Round(($currentTest / $totalTests) * 100, 1)
            Write-Host "    [$threads 线程] 进度: $currentTest/$totalTests ($progress%)" -ForegroundColor Gray
            
            $result = Run-Test -exe $program.Exe -dataSize $dataSize -threads $threads
            $time = $result.Time
            
            if ($time -eq $null) {
                Write-Host "    ? 运行失败" -ForegroundColor Red
                continue
            }
            
            $timeMs = [math]::Round($time * 1000, 2)
            $speedup = [math]::Round($serialTime / $time, 2)
            
            Write-Host "    ? 时间: $timeMs ms, 加速比: ${speedup}x" -ForegroundColor White
            
            # 写入结果文件
            "线程数: $threads" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
            "时间: $timeMs ms" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
            "加速比: ${speedup}x" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
            "" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
            
            # CSV记录
            "$($program.Name),$threads,$timeMs,$speedup" | Out-File -FilePath $csvResultFile -Append -Encoding UTF8
            
            # 汇总结果
            $summaryResults += @{
                Data = $dataName
                Size = $dataSize
                Program = $program.Name
                Threads = $threads
                TimeMs = $timeMs
                Speedup = $speedup
            }
        }
        
        "" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
    }
    
    Write-Host ""
}

# 生成总结报告
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
Write-Host "生成总结报告..." -ForegroundColor Cyan
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan

$summaryFile = Join-Path $resultDir "summary_report.txt"
$summaryCsvFile = Join-Path $resultDir "summary_all.csv"

# 总结文本报告
"=" * 80 | Out-File -FilePath $summaryFile -Encoding UTF8
"三对角矩阵求解器批量测试总结报告（内存数据生成版本）" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"生成时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"=" * 80 | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"" | Out-File -FilePath $summaryFile -Append -Encoding UTF8

# CSV汇总
"Data,Size,Program,Threads,Time(ms),Speedup" | Out-File -FilePath $summaryCsvFile -Encoding UTF8

foreach ($result in $summaryResults) {
    "$($result.Data),$($result.Size),$($result.Program),$($result.Threads),$($result.TimeMs),$($result.Speedup)" | 
        Out-File -FilePath $summaryCsvFile -Append -Encoding UTF8
}

# 按数据大小分组显示最佳性能
foreach ($dataConfig in $dataSizes) {
    $dataName = $dataConfig.Name
    $dataSize = $dataConfig.Size
    $dataResults = $summaryResults | Where-Object { $_.Data -eq $dataName }
    
    if ($dataResults.Count -eq 0) { continue }
    
    "数据集: $dataName (N=$dataSize)" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
    "-" * 80 | Out-File -FilePath $summaryFile -Append -Encoding UTF8
    
    # 串行时间
    $serialResult = $dataResults | Where-Object { $_.Program -eq "Sequential" }
    if ($serialResult) {
        "  串行基准: $($serialResult.TimeMs) ms" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
    }
    
    # 各算法最佳性能
    $algorithms = @("Brugnano", "RecursiveDoubling")
    foreach ($algo in $algorithms) {
        $algoResults = $dataResults | Where-Object { $_.Program -eq $algo }
        if ($algoResults.Count -gt 0) {
            $best = $algoResults | Sort-Object TimeMs | Select-Object -First 1
            $maxSpeedup = ($algoResults | Sort-Object Speedup -Descending | Select-Object -First 1).Speedup
            "  $algo 最佳: $($best.TimeMs) ms @ $($best.Threads) 线程 (最大加速比: ${maxSpeedup}x)" | 
                Out-File -FilePath $summaryFile -Append -Encoding UTF8
        }
    }
    
    "" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
}

"=" * 80 | Out-File -FilePath $summaryFile -Append -Encoding UTF8

Write-Host "? 测试完成!" -ForegroundColor Green
Write-Host ""
Write-Host "结果已保存至: $resultDir" -ForegroundColor Green
Write-Host "  - 各数据集详细结果: *_results.txt 和 *_results.csv" -ForegroundColor Gray
Write-Host "  - 总结报告: summary_report.txt" -ForegroundColor Gray
Write-Host "  - 汇总CSV: summary_all.csv" -ForegroundColor Gray
Write-Host ""
Write-Host "注意: 使用内存数据生成，避免了文件I/O瓶颈，测试速度显著提升！" -ForegroundColor Yellow
Write-Host ""
