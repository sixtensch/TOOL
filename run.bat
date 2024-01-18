@echo off

echo Executing run.bat

if [%1] == [] (set ConfigName=Debug) else set ConfigName=%1

pushd ".\build\%ConfigName%\"
start "TOOL (Test)" /b "TOOLTest.exe"
popd
