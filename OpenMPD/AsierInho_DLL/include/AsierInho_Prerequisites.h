// Define ASIERINHO version
#define ASIERINHO_VERSION_MAJOR 1
#define ASIERINHO_VERSION_MINOR 0
#define ASIERINHO_VERSION_PATCH 0
#define ASIERINHO_VERSION_SUFFIX ""
#define ASIERINHO_VERSION_NAME "Zoe"

#define ASIERINHO_VERSION    ((ASIERINHO_VERSION_MAJOR << 16) | (ASIERINHO_VERSION_MINOR << 8) | ASIERINHO_VERSION_PATCH)

#if defined( ASIERINHO_NONCLIENT_BUILD )
#    define _AsierInho_Export __declspec( dllexport )
#else
#    if defined( __MINGW32__ )
#        define _AsierInho_Export 
#    else
#        define _AsierInho_Export  __declspec( dllimport )
#    endif
#endif

#ifndef ASIERINHO_VEC3
#define ASIERINHO_VEC3
namespace AsierInho {
	enum BoardType { BensDesign = 0, AsiersDesign = 1 };
	enum AsierInhoMode { NORMAL = 0, MATD = 1 };
	
	typedef struct _vec3 {
		union {
			struct { float x, y, z; };
			struct { float r, g, b; };
		};
		_vec3() : x(0), y(0), z(0) { ; }
		_vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) { ; }
		_vec3(const _vec3& pt) : x(pt.x), y(pt.y), z(pt.z) {}
		_vec3& operator=(const _vec3& v) {
			if (this != &v) {
				x = v.x; y = v.y; z = v.z;
			}
			return *this;
		}
	} vec3;
};
#define AsierInho_Handler long long
#endif