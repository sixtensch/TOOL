@echo off

rmdir "package" /s /q
mkdir "package"
mkdir "package\TOOL"
mkdir "package\TOOL\include"
mkdir "package\TOOL\lib"
mkdir "package\TOOL\lib\Debug"
mkdir "package\TOOL\lib\Release"

call configure.bat -DCMAKE_BUILD_TYPE:STRING=Debug
call build.bat --config Debug 
xcopy "build\Debug\TOOL.lib" "package\TOOL\lib\Debug\" /y /q
xcopy "build\Debug\TOOL.pdb" "package\TOOL\lib\Debug\" /y /q

call configure.bat -DCMAKE_BUILD_TYPE:STRING=Release
call build.bat --config Release 
xcopy "build\Release\TOOL.lib" "package\TOOL\lib\Release\" /y /q

xcopy "include" "package\TOOL\include\" /y /e /q