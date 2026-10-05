@echo off
title GotchiLab_ // Servidor Web Serial Local
chcp 65001 >nul
cls

echo ======================================================================
echo    GotchiLab_ :: MediaLab_ UniOvi // Servidor Local Web Serial
echo ======================================================================
echo.
echo Iniciando servidor en http://localhost:8000/web/ ...
echo (Web Serial API requiere protocolo http://localhost o HTTPS).
echo.
echo Presiona Ctrl+C en esta ventana para detener el servidor.
echo.

timeout /t 1 /nobreak >nul
start "" "http://localhost:8000/web/"

python -m http.server 8000
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] No se pudo iniciar el servidor local con Python.
    echo Asegúrate de tener Python instalado y en el PATH de Windows.
    pause
)
