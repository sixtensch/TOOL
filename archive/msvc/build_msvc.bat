@echo off

if not defined VCToolsVersion call vcvarsall x64
if [%1] == [] (set ConfigName=Debug) else set ConfigName=%1

echo Building %ConfigName%

msbuild TOOL.vcxproj /p:configuration=%ConfigName% /v:m
msbuild Test.vcxproj /p:configuration=%ConfigName% /v:m