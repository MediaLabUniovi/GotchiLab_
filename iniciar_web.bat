@echo off
title GotchiLab_ // Servidor Web Serial Local
chcp 65001 >nul
cls

echo ======================================================================
echo    GotchiLab_ :: MediaLab_ UniOvi // Servidor Local Web Serial
echo ======================================================================
echo.

set "SERVER_CMD="

:: 1. Comprobar py launcher de Windows
py -3 --version >nul 2>&1
if %errorlevel% equ 0 (
    set SERVER_CMD=py -3 -m http.server 8000
    goto :found
)

:: 2. Comprobar ruta directa de Python en LocalAppData
if exist "%LOCALAPPDATA%\Python\bin\python.exe" (
    set SERVER_CMD="%LOCALAPPDATA%\Python\bin\python.exe" -m http.server 8000
    goto :found
)

:: 3. Comprobar python en PATH
python --version >nul 2>&1
if %errorlevel% equ 0 (
    set SERVER_CMD=python -m http.server 8000
    goto :found
)

:: 4. Comprobar Node.js
node --version >nul 2>&1
if %errorlevel% equ 0 (
    set SERVER_CMD=npx --yes http-server -p 8000
    goto :found
)

echo [ERROR] No se encontró Python ni Node.js disponible para iniciar el servidor local.
echo Por favor, asegúrate de tener Python instalado.
pause
exit /b 1

:found
echo Servidor listo. Abriendo http://localhost:8000/web/ en tu navegador...
echo (Web Serial API requiere protocolo http://localhost o HTTPS).
echo.
echo Presiona Ctrl+C en esta ventana para detener el servidor cuando termines.
echo.

timeout /t 1 /nobreak >nul
start "" "http://localhost:8000/web/"

%SERVER_CMD%
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] El servidor se detuvo con un error.
    pause
)
