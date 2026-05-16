@echo off
:: Monitor serie IncuTwin — COM18, 115200 baud
:: Pulsa Ctrl+C para salir
cd /d "%~dp0"
echo [IncuTwin] Abriendo monitor serie en COM18, 115200 baud...
pio device monitor --port COM18 --baud 115200 --filter colorize
