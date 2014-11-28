@echo off
mkdir ..\build
pushd ..\build
cl /Zi ..\code\main.c user32.lib gdi32.lib
popd