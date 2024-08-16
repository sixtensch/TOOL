
#ifndef _TOOL_EXT_H
#define _TOOL_EXT_H



//~ TOOL: the Tired-Of-Objects Library
//
// Function- and data-oriented standard library written in C++.
// Why not pure C? Because of namespaces, mathematical operator overloading, and templates.



//~ All flags:
// 
// TOOL_NO_ACRONYMS: Do not use TOOL acronyms (f32, v2, m4, etc) in the public namespace.
// TOOL_NO_MATH:     Do not use TOOL math functions or aliases in the public namespace.
// TOOL_NO_ALLOC:    Do not provide the ALLOC(...) macro.
// TOOL_NO_UTF:      Do not provide the UTF8(str) and UTF16(str) macros.



//~ Includes

// Basic types and helper functionality
#include "tool/basics.h"
#include "tool/color.h"
#include "tool/text.h"
#include "tool/memory.h"
#include "tool/variadic.h"
#include "tool/random.h"
#include "tool/linking.h"
#include "tool/exception.h"
#include "tool/threading.h"
#include "tool/temporal.h"
#include "tool/io.h"

// Mathematics and linear algebra
#include "tool/mathematics.h"
#include "tool/intrinsics.h"
#include "tool/vector.h"
#include "tool/matrix.h"
#include "tool/quaternion.h"
#include "tool/box.h"



#endif
