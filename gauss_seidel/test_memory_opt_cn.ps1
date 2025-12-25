# -*- coding: utf-8 -*-
# 访存优化性能测试脚本（中文版）
# 测试不同规模和线程数下的性能提升

# 设置控制台编码为UTF-8以支持中文
$OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
chcp 65001 > $null

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  访存优化性能测试" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# 编译优化版本
Write-Host "[1/3] 编译优化版本..." -ForegroundColor Yellow
make opt
if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败！" -ForegroundColor Red
    exit 1
}
Write-Host "编译完成`n" -ForegroundColor Green

# 测试配置
$sizes = @(256, 512, 1024)
$threads = @(1, 4, 8)

Write-Host "[2/3] 开始性能测试...`n" -ForegroundColor Yellow

foreach ($N in $sizes) {
    foreach ($t in $threads) {
        Write-Host "测试: N=$N, 线程数=$t" -ForegroundColor Cyan
        Write-Host "----------------------------------------"
        
        ./test_memory_optimization.exe $N $t
        
        Write-Host "`n"
    }
}

Write-Host "[3/3] 测试完成！`n" -ForegroundColor Green

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  关键发现总结" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "1. Tiling优化在大规模问题上效果最显著" -ForegroundColor White
Write-Host "2. 数据重排适合超大规模 + 高迭代次数" -ForegroundColor White
Write-Host "3. SIMD对红黑排序效果有限" -ForegroundColor White
Write-Host "4. 多线程 + Tiling 可获得最佳性能" -ForegroundColor White
Write-Host "`n详细报告见: MEMORY_OPTIMIZATION_REPORT.md`n" -ForegroundColor Yellow
