@echo off
:: IncuTwin - Compilar y flashear por fases
:: Uso: .\build_flash.bat [1|2|3]
set PORT=COM18
set FASE=%1

if "%FASE%"=="" (
    echo Uso: .\build_flash.bat [1^|2^|3]
    echo   1 = Bring-up hardware
    echo   2 = WiFi + ThingsBoard
    echo   3 = FSM + logica principal
    exit /b 0
)

cd /d "%~dp0"
echo.
echo [IncuTwin] Compilando Fase %FASE% ...
echo.

if "%FASE%"=="1" goto fase1
if "%FASE%"=="2" goto fase2
if "%FASE%"=="3" goto fase3
echo Fase invalida. Usa 1, 2 o 3.
exit /b 1

:fase1
pio run -e esp32dev
if errorlevel 1 ( echo ERROR: compilacion fallida & exit /b 1 )
echo.
echo [IncuTwin] Flasheando en %PORT% ...
pio run -e esp32dev -t upload --upload-port %PORT%
if errorlevel 1 ( echo ERROR: flash fallido & exit /b 1 )
echo.
echo [IncuTwin] Monitor serie (Ctrl+C para salir) ...
pio device monitor --port %PORT% --baud 115200
exit /b 0

:fase2
pio run -e esp32dev_phase2
if errorlevel 1 ( echo ERROR: compilacion fallida & exit /b 1 )
echo.
echo [IncuTwin] Flasheando en %PORT% ...
pio run -e esp32dev_phase2 -t upload --upload-port %PORT%
if errorlevel 1 ( echo ERROR: flash fallido & exit /b 1 )
echo.
echo [IncuTwin] Monitor serie (Ctrl+C para salir) ...
pio device monitor --port %PORT% --baud 115200
exit /b 0

:fase3
pio run -e esp32dev_phase3
if errorlevel 1 ( echo ERROR: compilacion fallida & exit /b 1 )
echo.
echo [IncuTwin] Flasheando en %PORT% ...
pio run -e esp32dev_phase3 -t upload --upload-port %PORT%
if errorlevel 1 ( echo ERROR: flash fallido & exit /b 1 )
echo.
echo [IncuTwin] Monitor serie (Ctrl+C para salir) ...
pio device monitor --port %PORT% --baud 115200
exit /b 0
