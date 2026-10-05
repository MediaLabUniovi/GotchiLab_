@echo off
setlocal enabledelayedexpansion

title GotchiLab_ - Pipeline de Compilacion y Despliegue Web

echo ===============================================================================
echo               GotchiLab_ :: Build ^& Web Deployment Pipeline
echo             Espressif ESP32 DevKit v1 - MediaLab_ STEAM Education
echo ===============================================================================
echo.

:: Detect Python
where python >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] No se ha encontrado Python en el sistema.
    echo Por favor, instale Python 3.8+ o anadalo al PATH.
    echo.
    pause
    exit /b 1
)

:: Ensure user Python scripts directory is on PATH
set "PY_SCRIPTS=%LOCALAPPDATA%\Packages\PythonSoftwareFoundation.Python.3.13_qbz5n2kfra8p0\LocalCache\local-packages\Python313\Scripts"
if exist "!PY_SCRIPTS!" (
    set "PATH=!PY_SCRIPTS!;%PATH%"
)

:: Also check default Python Scripts directory
for /f "delims=" %%I in ('python -c "import sys, pathlib; print(pathlib.Path(sys.executable).parent / 'Scripts')" 2^>nul') do (
    if exist "%%I" set "PATH=%%I;%PATH%"
)

echo [INFO] Verificando dependencias de compilacion...
python -c "import platformio" >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [SETUP] Instalando PlatformIO y esptool...
    python -m pip install platformio esptool
)

echo [INFO] Iniciando pipeline de compilacion por matriz de variantes...
echo.

python "%~dp0scripts\build_matrix.py"
if %ERRORLEVEL% neq 0 (
    echo.
    echo ===============================================================================
    echo [ERROR] La compilacion ha fallado. Revise los mensajes de error superiores.
    echo ===============================================================================
    echo.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ===============================================================================
echo [EXITO] Despliegue completado con exito.
echo         - Binarios unificados generados en: web\binaries\
echo         - Manifiesto actualizado en:        web\manifest.json
echo         - Animaciones sincronizadas en:     web\animations.js
echo.
echo Para probar localmente la aplicacion Web Flasher, puede ejecutar:
echo   python -m http.server 8000 --directory web
echo y abrir en su navegador Chrome/Edge: http://localhost:8000
echo ===============================================================================
echo.
pause
