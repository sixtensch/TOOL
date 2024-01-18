@echo off

echo Executing debug.bat

if [%1] == [] (set ConfigName=Debug) else set ConfigName=%1

pushd ".\build\%ConfigName%\"
call remedy TOOLTest.exe
popd
