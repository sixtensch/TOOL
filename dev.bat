@echo off

if not defined VCToolsVersion call vcvarsall x64
call 4ed.bat . -W

$wshell = New-Object -ComObject wscript.shell
$wshell.AppActivate('4coder')