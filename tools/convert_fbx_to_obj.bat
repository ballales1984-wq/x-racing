@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%~dp0..
set FBX=%~1
set OBJ=%~2

if "%FBX%"=="" set FBX=%PROJECT_ROOT%\assets\models\car.fbx
if "%OBJ%"=="" set OBJ=%PROJECT_ROOT%\assets\models\car_converted.obj

echo Converting "%FBX%" -^> "%OBJ%"

python "%SCRIPT_DIR%convert_fbx_to_obj_assimp.py" "%FBX%" "%OBJ%"
set RESULT=%ERRORLEVEL%

if %RESULT% neq 0 (
    echo ERROR: Conversion failed with exit code %RESULT%
    exit /b %RESULT%
)

echo Conversion complete.
