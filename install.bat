@echo off
chcp 65001 > nul
setlocal
rem Установка cryptum: копирование в Program Files и добавление папки в системную переменную Path.
rem Требуются права администратора.

net session > nul 2>&1
if errorlevel 1 (
    echo Запустите install.bat от имени администратора: правой кнопкой мыши - "Запуск от имени администратора".
    exit /b 1
)

set SOURCE=%~dp0bin
set TARGET=%ProgramFiles%\Cryptum

if not exist "%SOURCE%\cryptum.exe" (
    echo Сначала соберите проект: build.bat
    exit /b 1
)

if not exist "%TARGET%" mkdir "%TARGET%"
copy /Y "%SOURCE%\cryptum.exe" "%TARGET%\" > nul || exit /b 1
copy /Y "%SOURCE%\atbash.dll" "%TARGET%\" > nul || exit /b 1
copy /Y "%SOURCE%\transposition.dll" "%TARGET%\" > nul || exit /b 1
copy /Y "%SOURCE%\tea.dll" "%TARGET%\" > nul || exit /b 1

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$target = Join-Path $env:ProgramFiles 'Cryptum';" ^
  "$path = [Environment]::GetEnvironmentVariable('Path', 'Machine');" ^
  "$parts = $path -split ';' | Where-Object { $_ -ne '' };" ^
  "if ($parts -notcontains $target) { [Environment]::SetEnvironmentVariable('Path', (($parts + $target) -join ';'), 'Machine') }"
if errorlevel 1 exit /b 1

echo Программа установлена в "%TARGET%".
echo Откройте новое окно PowerShell и выполните: cryptum
endlocal
