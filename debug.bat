@echo off

echo Executing debug.bat

if [%1] == [] (set ConfigName=Debug) else set ConfigName=%1

::pushd ".\Bin\Test-x64-%ConfigName%\"
pushd ".\build\Debug\"
call remedy TOOLTest.exe
popd
