@echo off
chcp 65001 > nul
setlocal
rem Удаление cryptum: удаление папки из Program Files и из системной переменной Path.
rem Требуются права администратора.

net session > nul 2>&1
if errorlevel 1 (
    echo Запустите uninstall.bat от имени администратора.
    exit /b 1
)

set TARGET=%ProgramFiles%\Cryptum

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$target = Join-Path $env:ProgramFiles 'Cryptum';" ^
  "$path = [Environment]::GetEnvironmentVariable('Path', 'Machine');" ^
  "$parts = $path -split ';' | Where-Object { $_ -ne '' -and $_ -ne $target };" ^
  "[Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'Machine')"

if exist "%TARGET%" rmdir /S /Q "%TARGET%"
echo Программа cryptum удалена.
endlocal
