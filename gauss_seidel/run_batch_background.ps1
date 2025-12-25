# 后台运行 batch_test_all.ps1（类似 nohup）
# 输出会保存到日志文件

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

$logFile = "batch_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  后台运行批量测试" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "日志文件: $logFile" -ForegroundColor Yellow
Write-Host "监控命令: Get-Content $logFile -Wait -Tail 20" -ForegroundColor Yellow
Write-Host ""

# 方法1: 使用 Start-Process（新窗口，可最小化）
Write-Host "选择运行方式:" -ForegroundColor Green
Write-Host "  1. 新窗口运行（推荐，可最小化）" -ForegroundColor White
Write-Host "  2. 完全后台运行（无窗口，通过日志查看）" -ForegroundColor White
Write-Host "  3. 当前窗口运行" -ForegroundColor White
Write-Host ""

$choice = Read-Host "请选择 (1/2/3)"

switch ($choice) {
    "1" {
        # 新窗口运行，可以最小化
        Write-Host "在新窗口启动测试..." -ForegroundColor Green
        Start-Process powershell -ArgumentList "-NoExit", "-Command", `
            "& { cd '$scriptPath'; .\batch_test_all.ps1 | Tee-Object -FilePath '$logFile' }"
        Write-Host ""
        Write-Host "? 已在新窗口启动！" -ForegroundColor Green
        Write-Host "  可以最小化窗口，测试会继续运行" -ForegroundColor Yellow
        Write-Host "  查看实时日志: Get-Content $logFile -Wait -Tail 20" -ForegroundColor Cyan
    }
    "2" {
        # 完全后台运行
        Write-Host "启动后台作业..." -ForegroundColor Green
        $job = Start-Job -ScriptBlock {
            param($scriptPath, $logFile)
            Set-Location $scriptPath
            & .\batch_test_all.ps1 *>&1 | Out-File -FilePath $logFile -Encoding UTF8
        } -ArgumentList $scriptPath, $logFile
        
        Write-Host ""
        Write-Host "? 后台作业已启动！" -ForegroundColor Green
        Write-Host "  作业ID: $($job.Id)" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "管理命令:" -ForegroundColor Cyan
        Write-Host "  查看状态: Get-Job -Id $($job.Id)" -ForegroundColor White
        Write-Host "  查看实时日志: Get-Content $logFile -Wait -Tail 20" -ForegroundColor White
        Write-Host "  停止作业: Stop-Job -Id $($job.Id)" -ForegroundColor White
        Write-Host "  获取结果: Receive-Job -Id $($job.Id)" -ForegroundColor White
    }
    "3" {
        # 当前窗口运行（带日志）
        Write-Host "在当前窗口运行..." -ForegroundColor Green
        .\batch_test_all.ps1 | Tee-Object -FilePath $logFile
    }
    default {
        Write-Host "无效选择，使用方法1（新窗口）" -ForegroundColor Yellow
        Start-Process powershell -ArgumentList "-NoExit", "-Command", `
            "& { cd '$scriptPath'; .\batch_test_all.ps1 | Tee-Object -FilePath '$logFile' }"
        Write-Host ""
        Write-Host "? 已在新窗口启动！" -ForegroundColor Green
    }
}

Write-Host ""
