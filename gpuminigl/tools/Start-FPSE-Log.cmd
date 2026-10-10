@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Receive-FPSELog.ps1" -OutputDirectory "%~dp0."
if errorlevel 1 pause
