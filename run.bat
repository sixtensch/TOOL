@echo off

echo Executing run.bat

if [%1] == [] (set ConfigName=Debug) else set ConfigName=%1

start /D ".\Bin\Test-x64-%ConfigName%\" "TOOL (Test)" ".\Bin\Test-x64-%ConfigName%\Test.exe"
