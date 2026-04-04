@echo off
echo ================================================
echo   4WD Robot Server - Port 25565
echo   Public IP: Check at ifconfig.me
echo   Dashboard: http://localhost:25565
echo   Token: robot123
echo ================================================
echo.

cd /d "%~dp0"
"C:\Users\tanap\AppData\Local\Programs\Python\Python313\python.exe" -c "import flask" 2>nul
if errorlevel 1 (
    echo Installing Flask...
    "C:\Users\tanap\AppData\Local\Programs\Python\Python313\python.exe" -m pip install flask
)

set PORT=25565
set ROBOT_API_TOKEN=robot123
"C:\Users\tanap\AppData\Local\Programs\Python\Python313\python.exe" app.py
pause
