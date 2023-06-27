rm build -rf
cmake -S . -B Build -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang -D CMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles" "$@"