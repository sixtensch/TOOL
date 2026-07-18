#ifndef _TOOL_UTILITY_H
#define _TOOL_UTILITY_H



//- Macros

#define TOOL_CONCAT(a, b) TOOL_CONCAT_INNER(a, b)
#define TOOL_CONCAT_INNER(a, b) a ## b
#define TOOL_UNIQUE_NAME(base) TOOL_CONCAT(base, __COUNTER__)

// Statement called on destruction of temporary variable at the end of the current scope, in reverse order
#define TOOL_DEFER(statement) Tool::DestructCaller TOOL_UNIQUE_NAME(_defer_)([&]() { statement ; }) 



namespace Tool
{

    // - Deferral utilities

    template<typename T_func>
    struct DestructCaller
    {
        T_func destruct;

        DestructCaller(T_func function) : destruct(function) { }
        ~DestructCaller() { destruct(); }
    };

}

#endif