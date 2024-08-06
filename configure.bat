@echo off

call rmdir build /s /q
call cmake -S . -B build %*

:: -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang -T ClangCL