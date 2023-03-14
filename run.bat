@echo off

echo Executing run.bat

if [%1] == [] (set ConfigName=Debug) else set ConfigName=%1

start /D ".\Bin\TOOL-x64-%ConfigName%\" "TOOL (Running)" ".\Bin\TOOL-x64-%ConfigName%\TOOL.exe"
