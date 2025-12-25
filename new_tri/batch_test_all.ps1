# 批量测试三对角矩阵求解器脚本
# 测试串行版本和两个并行版本（Brugnano 和 Recursive Doubling）
# 不同数据大小和线程数

# 切换到脚本所在目录
Set-Location -Path $PSScriptRoot

# 设置编码为 UTF-8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 创建结果目录
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultDir = "batch_results_$timestamp"
New-Item -ItemType Directory -Force -Path $resultDir | Out-Null

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "批量测试三对角矩阵求解器" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "结果目录: $resultDir" -ForegroundColor Green
Write-Host ""

# 编译所有程序
Write-Host "正在编译程序..." -ForegroundColor Yellow
try {
    # 编译串行版本
    Write-Host "  [1/3] 编译 sequential_solver..." -ForegroundColor Gray
    g++ -fopenmp -O2 -std=c++11 -Wall -o sequential_solver_original.exe sequential_solver.cpp
    if ($LASTEXITCODE -ne 0) { throw "sequential_solver 编译失败" }
    
    # 编译 Brugnano 版本
    Write-Host "  [2/3] 编译 openmp_brugnano..." -ForegroundColor Gray
    g++ -fopenmp -O2 -std=c++11 -Wall -o openmp_brugnano.exe openmp_brugnano.cpp
    if ($LASTEXITCODE -ne 0) { throw "openmp_brugnano 编译失败" }
    
    # 编译 Recursive Doubling 版本
    Write-Host "  [3/3] 编译 openmp_recursive_doubling..." -ForegroundColor Gray
    g++ -fopenmp -O2 -std=c++11 -Wall -o openmp_recursive_doubling.exe openmp_recursive_doubling.cpp
    if ($LASTEXITCODE -ne 0) { throw "openmp_recursive_doubling 编译失败" }
    
    Write-Host "? 所有程序编译成功!" -ForegroundColor Green
    Write-Host ""
} catch {
    Write-Host "? 编译失败: $_" -ForegroundColor Red
    exit 1
}

# 定义测试数据文件
$dataFiles = @(
    "inputs/test_8k.txt",
    "inputs/test_16k.txt",
    "inputs/test_128k.txt",
    "inputs/test_1024k.txt",
    "inputs/test_4M.txt"
)

# 定义线程数配置（不包括串行）
$threadCounts = @(1, 2, 4, 8, 10, 16, 20)

# 测试程序列表
$programs = @(
    @{Name="Sequential"; Exe="sequential_solver_original.exe"; IsSerial=$true},
    @{Name="Brugnano"; Exe="openmp_brugnano.exe"; IsSerial=$false},
    @{Name="RecursiveDoubling"; Exe="openmp_recursive_doubling.exe"; IsSerial=$false}
)

# 函数：从程序输出中提取时间（秒）
function Extract-Time {
    param([string]$output)
    
    # 匹配 "求解时间: X.XXXXXX 秒" 格式
    if ($output -match "求解时间:\s+([\d.]+)\s+秒") {
        return [double]$matches[1]
    }
    return $null
}

# 函数：运行单个测试
function Run-Test {
    param(
        [string]$exe,
        [string]$inputFile,
        [int]$threads = 1  # 默认1个线程
    )
    
    # 运行程序并捕获输出（传递线程数作为第二个参数）
    $output = & ".\$exe" $inputFile $threads 2>&1 | Out-String
    
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

$totalTests = $dataFiles.Count * (1 + 2 * $threadCounts.Count)
$currentTest = 0

# 汇总结果数组
$summaryResults = @()

foreach ($dataFile in $dataFiles) {
    if (!(Test-Path $dataFile)) {
        Write-Host "警告: 数据文件不存在: $dataFile" -ForegroundColor Yellow
        continue
    }
    
    # 提取数据大小标识
    $dataName = [System.IO.Path]::GetFileNameWithoutExtension($dataFile)
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
    Write-Host "测试数据: $dataName" -ForegroundColor Cyan
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
    
    $serialResult = Run-Test -exe "sequential_solver_original.exe" -inputFile $dataFile
    $serialTime = $serialResult.Time
    
    if ($serialTime -eq $null) {
        Write-Host "    ? 串行版本运行失败" -ForegroundColor Red
        continue
    }
    
    $serialTimeMs = [math]::Round($serialTime * 1000, 2)
    Write-Host "    ? 串行时间: $serialTimeMs ms" -ForegroundColor White
    
    # 写入结果文件
    "=" * 60 | Out-File -FilePath $dataResultFile -Encoding UTF8
    "数据文件: $dataName" | Out-File -FilePath $dataResultFile -Append -Encoding UTF8
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
            
            $result = Run-Test -exe $program.Exe -inputFile $dataFile -threads $threads
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
"三对角矩阵求解器批量测试总结报告" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"生成时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"=" * 80 | Out-File -FilePath $summaryFile -Append -Encoding UTF8
"" | Out-File -FilePath $summaryFile -Append -Encoding UTF8

# CSV汇总
"Data,Program,Threads,Time(ms),Speedup" | Out-File -FilePath $summaryCsvFile -Encoding UTF8

foreach ($result in $summaryResults) {
    "$($result.Data),$($result.Program),$($result.Threads),$($result.TimeMs),$($result.Speedup)" | 
        Out-File -FilePath $summaryCsvFile -Append -Encoding UTF8
}

# 按数据大小分组显示最佳性能
foreach ($dataFile in $dataFiles) {
    $dataName = [System.IO.Path]::GetFileNameWithoutExtension($dataFile)
    $dataResults = $summaryResults | Where-Object { $_.Data -eq $dataName }
    
    if ($dataResults.Count -eq 0) { continue }
    
    "数据集: $dataName" | Out-File -FilePath $summaryFile -Append -Encoding UTF8
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
            "  $algo 最佳: $($best.TimeMs) ms @ $($best.Threads) 线程 (加速比: $($best.Speedup)x)" | 
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
