@echo off

if not defined VCToolsVersion call vcvarsall x64
call 4ed.bat . -W

@REM $wshell = New-Object -ComObject wscript.shell
@REM $wshell.AppActivate('4coder')
