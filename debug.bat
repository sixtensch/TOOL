@echo off

echo Executing debug.bat

if [%1] == [] (set ConfigName=Debug) else set ConfigName=%1

pushd ".\Bin\TOOL-x64-%ConfigName%\"
call remedy TOOL.exe
popd
