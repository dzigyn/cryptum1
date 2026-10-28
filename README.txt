Multi-Algo Cryptotool (cryptum) — инструкция по запуску в VS Code (Windows)
=========================================================================

Состав папки
------------
  include\        общие заголовки: интерфейс библиотек (crypto_api.h), очистка памяти
  src\            основная программа: меню, ввод с клавиатуры, выбор файлов, загрузка библиотек
  ciphers\        три динамические библиотеки: atbash, transposition, tea
  tests\          программа автоматических тестов
  files\input\    готовые файлы для шифрования (тексты и картинка)
  files\keys\     готовые ключи для контрольного примера из отчёта
  files\output\   сюда программа складывает результаты (создаётся сама)
  build.bat       сборка в Windows
  Makefile        сборка через make (mingw32-make в Windows, make в Linux)
  install.bat / uninstall.bat   установка в Program Files и PATH (от имени администратора)
  prebuilt-windows\             уже собранные cryptum.exe, tests.exe и .dll (запасной вариант)

ШАГ 0. Один раз: компилятор g++ (если ещё не установлен)
---------------------------------------------------------
  Проверка: в терминале введи  g++ --version
  Если «не является внутренней или внешней командой»:
    1. Скачай и установи MSYS2 с сайта https://www.msys2.org (папка C:\msys64).
    2. В окне MSYS2 выполни:  pacman -S --needed mingw-w64-ucrt-x86_64-gcc   (на вопросы — Y)
    3. Пуск → «Изменение системных переменных среды» → «Переменные среды» → Path → «Изменить» →
       «Создать» → C:\msys64\ucrt64\bin → ОК, ОК, ОК.
    4. Закрой VS Code, открой заново и снова проверь g++ --version.

ШАГ 1. Открыть проект
---------------------
  1. Распакуй архив. Внутри будет папка cryptum — открой именно её:
     VS Code → File → Open Folder → папка, где ПРЯМО ВНУТРИ лежат build.bat и src.
  2. Меню Terminal → New Terminal.

ШАГ 2. Собрать
--------------
  Ctrl+Shift+B  или в терминале:  .\build.bat
  В конце — «Сборка завершена». В папке bin: cryptum.exe, tests.exe, atbash.dll, transposition.dll, tea.dll.

ШАГ 3. Запустить программу
--------------------------
  .\bin\cryptum.exe
  Откроется меню. Вводи номер пункта и нажимай Enter.
  Запускай из папки проекта: тогда программа видит папки files\input, files\keys, files\output.

  Тесты:  .\bin\tests.exe   — 9 строк [ OK ] и «ошибок: 0».

Если сборка не получилась
-------------------------
  Запусти готовую версию:  .\prebuilt-windows\cryptum.exe   и пришли текст ошибки сборки.

Установка в систему (по ТЗ)
---------------------------
  Правой кнопкой по install.bat → «Запуск от имени администратора».
  Потом в новом окне PowerShell команда  cryptum  запускает программу из любой папки.
  Удаление: uninstall.bat от имени администратора.

Git: ветки (ТЗ п. 4.4.9)
------------------------
  git init
  git add .
  git commit -m "Первая версия cryptum"
  git branch -M main
  git remote add origin https://github.com/dzigyn/cryptum.git
  git push -u origin main
  Новые изменения — в отдельной ветке:  git checkout -b feature/имя  → commit → push → Pull request в main.
