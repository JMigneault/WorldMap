@echo off
pushd build
cl /I ..\libraries\SDL2\include /I ..\libraries\SDL2_ttf\include ..\source\game.cpp /Zi /link /libpath "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.18362.0\um\x64\shell32.lib" /libpath ..\libraries\SDL2\lib\x64\SDL2main.lib /libpath ..\libraries\SDL2\lib\x64\SDL2.lib /libpath ..\libraries\SDL2\lib\x64\SDL2test.lib /libpath ..\libraries\SDL2_ttf\lib\x64\SDL2_ttf.lib /SUBSYSTEM:WINDOWS
popd
