#pragma once

// Standard base includes, defines that indicate our current platform, etc.

#include <stddef.h>
#if _MSC_VER
	#define UNITY_WIN 1
#endif
// Which graphics device APIs we possibly support?
#if UNITY_WIN
	
	#define SUPPORT_OPENGL_UNIFIED 1
	#define SUPPORT_OPENGL_CORE 1
#endif
// COM-like Release macro
#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(a) if (a) { a->Release(); a = NULL; }
#endif

