rm build -rf

if [ "$MSYSTEM" = "MINGW64" ];
then
    TOOL_GENERATOR="MinGW Makefiles"
    TOOL_COMPILER="clang++"
else
    TOOL_GENERATOR="Unix Makefiles"
    TOOL_COMPILER="clang++"
fi

cmake -S . -B build -D CMAKE_CXX_COMPILER="$TOOL_COMPILER" -D CMAKE_BUILD_TYPE=Debug -G "$TOOL_GENERATOR" "$@"
