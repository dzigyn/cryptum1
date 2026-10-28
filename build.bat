@echo off
chcp 65001 > nul
setlocal
cd /d "%~dp0"

where g++ > nul 2> nul
if errorlevel 1 (
    echo Ошибка: компилятор g++ не найден. Установите MinGW-w64 и добавьте его папку bin в PATH.
    exit /b 1
)

if "%1"=="clean" (
    if exist bin rmdir /S /Q bin
    if exist build rmdir /S /Q build
    echo Собранные файлы удалены.
    exit /b 0
)

set FLAGS=-std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude -Isrc -Iciphers/common
set RUNTIME=-static-libgcc -static-libstdc++
if not exist bin mkdir bin
if not exist build mkdir build

echo [1/5] Библиотека atbash.dll
g++ %FLAGS% -shared ciphers\atbash\atbash.cpp -o bin\atbash.dll -lbcrypt %RUNTIME% || exit /b 1

echo [2/5] Библиотека transposition.dll
g++ %FLAGS% -shared ciphers\transposition\transposition.cpp -o bin\transposition.dll -lbcrypt %RUNTIME% || exit /b 1

echo [3/5] Библиотека tea.dll
g++ %FLAGS% -shared ciphers\tea\tea.cpp -o bin\tea.dll -lbcrypt %RUNTIME% || exit /b 1

echo [4/5] Основная программа cryptum.exe
g++ %FLAGS% src\main.cpp src\menu.cpp src\console_ui.cpp src\file_browser.cpp src\platform.cpp src\library_loader.cpp src\byte_io.cpp src\crypto_operations.cpp -o bin\cryptum.exe %RUNTIME% || exit /b 1

echo [5/5] Программа тестов tests.exe
g++ %FLAGS% tests\cipher_tests.cpp src\platform.cpp src\library_loader.cpp -o bin\tests.exe %RUNTIME% || exit /b 1

echo.
echo Сборка завершена. Файлы находятся в папке bin.
echo   Запуск:   .\bin\cryptum.exe
echo   Тесты:    .\bin\tests.exe
if "%1"=="test" bin\tests.exe
endlocal
