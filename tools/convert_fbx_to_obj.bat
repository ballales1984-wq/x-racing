@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set FBX=%~1
set OBJ=%~2

if "%FBX%"=="" set FBX=%SCRIPT_DIR%assets\models\car.fbx
if "%OBJ%"=="" set OBJ=%SCRIPT_DIR%assets\models\car_converted.obj

echo Converting "%FBX%" -> "%OBJ%"

D:\blender.exe --background --python "%SCRIPT_DIR%tools\convert_fbx_to_obj.py" -- "%FBX%" "%OBJ%"
set RESULT=%ERRORLEVEL%

if %RESULT% neq 0 (
    echo ERROR: Blender conversion failed with exit code %RESULT%
    pause
    exit /b %RESULT%
)

echo Conversion complete.
