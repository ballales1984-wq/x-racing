@echo off
set FBX=%~1
set OBJ=%~2

if "%FBX%"=="" set FBX=assets\models\car.fbx
if "%OBJ%"=="" set OBJ=assets\models\car_converted.obj

D:\blender.exe --background --python tools\convert_fbx_to_obj.py -- "%FBX%" "%OBJ%"

echo Conversion complete.
pause
