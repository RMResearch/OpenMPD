// Define GSPAT version
#define GSPAT_VERSION_MAJOR 1
#define GSPAT_VERSION_MINOR 0
#define GSPAT_VERSION_PATCH 0
#define GSPAT_VERSION_SUFFIX ""
#define GSPAT_VERSION_NAME "Zoe"

#define GSPAT_VERSION    ((GSPAT_VERSION_MAJOR << 16) | (GSPAT_VERSION_MINOR << 8) | GSPAT_VERSION_PATCH)

#if defined( GSPAT_NONCLIENT_BUILD )
#    define _GSPAT_Export __declspec( dllexport )
#else
#    if defined( __MINGW32__ )
#        define _GSPAT_Export 
#    else
#        define _GSPAT_Export  __declspec( dllimport )
#    endif
#endif
#define GSPAT_Solver_Handler long long
#define GSPAT_Solution_Handler long long

#ifndef GSPAT_MATRIXALIGNMENT
#define GSPAT_MATRIXALIGNMENT
namespace GSPAT {
	enum MatrixAlignment { ColumnMajorAlignment = 0, RowMajorAlignment = 1 };
};
#endif


