@echo off
mkdir ..\build
pushd ..\build
cl /Z7 /Zi /Zl ..\code\main.c
popd