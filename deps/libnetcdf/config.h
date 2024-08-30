#ifndef NETCDF_CONFIG_H
#define NETCDF_CONFIG_H

#define PACKAGE "netcdf-c"
#define PACKAGE_BUGREPORT "support-netcdf@unidata.ucar.edu"
#define PACKAGE_NAME "netCDF"
#define PACKAGE_STRING "netCDF 4.8.1"
#define PACKAGE_TARNAME "netcdf-c"
#define PACKAGE_URL ""
#define PACKAGE_VERSION "4.8.1"
#define REMOTETESTSERVERS "remotetest.unidata.ucar.edu"
#define VERSION "4.8.1"

#ifdef LINUX
#define CHUNK_CACHE_NELEMS 4133
#define CHUNK_CACHE_PREEMPTION 0.75
#define CHUNK_CACHE_SIZE 16777216
#define CHUNK_CACHE_SIZE_NCZARR 4194304
#define DEFAULT_CHUNKS_IN_CACHE 10
#define DEFAULT_CHUNK_SIZE 4194304
#define ENABLE_CDF5 1
#define ENABLE_DAP 1
#define ENABLE_DAP4 1
#define ENABLE_DAP_REMOTE_TESTS 1
#define ENABLE_SET_LOG_LEVEL 1
#define H5_USE_16_API 1
#define HAS_MULTIFILTERS 1
#define HAVE_ALLOCA 1
#define HAVE_ALLOCA_H 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_CURLINFO_RESPONSE_CODE 1
#define HAVE_CURLOPT_BUFFERSIZE 1
#define HAVE_CURLOPT_KEEPALIVE 1
#define HAVE_CURLOPT_KEYPASSWD 1
#define HAVE_CURLOPT_PASSWORD 1
#define HAVE_CURLOPT_USERNAME 1
#define HAVE_DECL_ISFINITE 1
#define HAVE_DECL_ISINF 1
#define HAVE_DECL_ISNAN 1
#define HAVE_DIRENT_H 1
#define HAVE_DLFCN_H 1
#define HAVE_EXECINFO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_FILENO 1
#define HAVE_FSYNC 1
#define HAVE_FTW_H 1
#define HAVE_GETOPT_H 1
#define HAVE_GETPAGESIZE 1
#define HAVE_GETRLIMIT 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_H5ALLOCATE_MEMORY 1
#define HAVE_H5FREE_MEMORY 1
#define HAVE_H5PSET_DEFLATE 1
#define HAVE_H5RESIZE_MEMORY 1
#define HAVE_HDF5_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIBGEN_H 1
#define HAVE_LIBM 1
#define HAVE_LOCALE_H 1
#define HAVE_LONG_LONG_INT 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMORY_H 1
#define HAVE_MKSTEMP 1
#define HAVE_MKTEMP 1
#define HAVE_MMAP 1
#define HAVE_MREMAP 1
#define HAVE_RANDOM 1
#define HAVE_SIZE_T 1
#define HAVE_SNPRINTF 1
#define HAVE_SSIZE_T 1
#define HAVE_STDARG_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_ST_BLKSIZE 1
#define HAVE_SYSCONF 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H 1
#define HAVE_UINT 1
#define HAVE_UINT64_T 1
#define HAVE_UINTPTR_T 1
#define HAVE_UNISTD_H 1
#define HAVE_UNSIGNED_LONG_LONG_INT 1
#define HAVE_USHORT 1
#define HDF5_SUPPORTS_PAR_FILTERS 1
#define HDF5_UTF8_PATHS 1
#define LT_OBJDIR ".libs/"
#define MAX_DEFAULT_CACHE_SIZE 67108864
#define NCIO_MINBLOCKSIZE 256
#define NCPROPERTIES_EXTRA ""
#define NC_DISPATCH_VERSION 3
#define SIZEOF_DOUBLE 8
#define SIZEOF_FLOAT 4
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_OFF_T 8
#define SIZEOF_SHORT 2
#define SIZEOF_SIZE_T 8
#define SIZEOF_SSIZE_T 8
#define SIZEOF_UINT 4
#define SIZEOF_UNSIGNED_CHAR 1
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_USHORT 2
#define SIZEOF_VOIDP 8
#define STDC_HEADERS 1
#define TEMP_LARGE "."
#define USE_DAP 1
#define USE_EXTREME_NUMBERS 1
#define USE_HDF5 1
#define USE_NETCDF4 1
#define USE_NETCDF_2 1

#include <gnu/libc-version.h>
#include <features.h>
#if __GLIBC_PREREQ(2, 38)
#define HAVE_STRLCAT 1
#endif
#endif

#ifdef WIN32
#ifdef _MSC_VER

#if _MSC_VER >= 1900
#define STDC99
#endif

#include <io.h>
#pragma warning(disable : 4018 4996 4244 4305)
#define unlink _unlink
#define open _open
#define close _close
#define read _read
#define lseek _lseeki64
#define fstat _fstat64
#define off_t __int64
#define _off_t __int64
#define _OFF_T_DEFINED

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef strincasecmp
#define strncasecmp _strnicmp
#endif

#ifndef snprintf
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#endif

#define strdup _strdup
#define fdopen _fdopen
#define write _write
#define strtoll _strtoi64
#endif /*_MSC_VER */

#define CHUNK_CACHE_NELEMS 4133
#define CHUNK_CACHE_PREEMPTION 0.75
#define CHUNK_CACHE_SIZE 16777216
#define CHUNK_CACHE_SIZE_NCZARR 4194304
#define DEFAULT_CHUNKS_IN_CACHE 10
#define DEFAULT_CHUNK_SIZE 16777216
#define ENABLE_CDF5 1
#define ENABLE_DAP 1
#define ENABLE_DAP4 1
#define ENABLE_DAP_REMOTE_TESTS 1
#define H5_USE_16_API 1
#define HAS_MULTIFILTERS 1
#define HAVE_DECL_ISFINITE 1
#define HAVE_DECL_ISINF 1
#define HAVE_DECL_ISNAN 1
#define HAVE_FCNTL_H 1
#define HAVE_BASETSD_H 1
#define HAVE_FILE_LENGTH_I64 1
#define HAVE_FILENO 1
#define HAVE_HDF5_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LOCALE_H 1
#define HAVE_LONG_LONG_INT 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMMOVE 1
#define HAVE_MKTEMP 1
#define HAVE_SNPRINTF 1
#define HAVE_PTRDIFF_T 1
#define HAVE_UINTPTR_T 1
#define HAVE_STDARG_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRDUP 1
#define HAVE_SIGNAL_H 1
#define HAVE_STRING_H 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H 1
#define HAVE_UINT64_T 1
#define HDF5_SUPPORTS_PAR_FILTERS 1
#define HDF5_UTF8_PATHS 1
#define YY_NO_UNISTD_H 1
#define ENABLE_SET_LOG_LEVEL 1
#define MAX_DEFAULT_CACHE_SIZE 67108864
#define NCIO_MINBLOCKSIZE 256
#define NC_DISPATCH_VERSION 3
#define BUILD_V2 1
#define USE_X_GETOPT 1
#define ENABLE_EXTREME_NUMBERS 1
#define SIZEOF_ULONGLONG 8
#define SIZEOF_LONGLONG 8
#define SIZEOF_CHAR 1
#define SIZEOF_UCHAR 1
#define SIZEOF_VOIDSTAR 8
#define SIZEOF_DOUBLE 8
#define SIZEOF_FLOAT 4
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_OFF_T 8
#define SIZEOF_SHORT 2
#define SIZEOF_SIZE_T 8
#define SIZEOF_UNSIGNED_INT 4
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_UNSIGNED_SHORT_INT 2
#define SIZEOF_VOIDP 8
#define TEMP_LARGE "."
#define USE_DAP 1
#define USE_EXTREME_NUMBERS 1
#define USE_HDF5 1
#define USE_NETCDF4 1
#define USE_NETCDF_2 1

#ifndef HAVE_STRCASECMP
#define strcasecmp _stricmp
#endif

#ifndef HAVE_STRNCASECMP
#define strncasecmp _strnicmp
#endif

#ifndef HAVE_SNPRINTF
#define snprintf _snprintf
#endif

#endif // WIN32

#ifdef DARWIN
#define CHUNK_CACHE_NELEMS 4133
#define CHUNK_CACHE_PREEMPTION 0.75
#define CHUNK_CACHE_SIZE 16777216
#define CHUNK_CACHE_SIZE_NCZARR 4194304
#define DEFAULT_CHUNKS_IN_CACHE 10
#define DEFAULT_CHUNK_SIZE 4194304
#define ENABLE_CDF5 1
#define ENABLE_DAP 1
#define ENABLE_DAP4 1
#define ENABLE_DAP_REMOTE_TESTS 1
#define ENABLE_NCZARR_ZIP 1
#define ENABLE_SET_LOG_LEVEL 1
#define H5_USE_16_API 1
#define HAS_MULTIFILTERS 1
#define HAVE_ALLOCA 1
#define HAVE_ALLOCA_H 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_CURLINFO_RESPONSE_CODE 1
#define HAVE_CURLOPT_BUFFERSIZE 1
#define HAVE_CURLOPT_KEEPALIVE 1
#define HAVE_CURLOPT_KEYPASSWD 1
#define HAVE_CURLOPT_PASSWORD 1
#define HAVE_CURLOPT_USERNAME 1
#define HAVE_DECL_ISFINITE 1
#define HAVE_DECL_ISINF 1
#define HAVE_DECL_ISNAN 1
#define HAVE_DIRENT_H 1
#define HAVE_DLFCN_H 1
#define HAVE_EXECINFO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_FILENO 1
#define HAVE_FSYNC 1
#define HAVE_FTW_H 1
#define HAVE_GETOPT_H 1
#define HAVE_GETPAGESIZE 1
#define HAVE_GETRLIMIT 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_H5ALLOCATE_MEMORY 1
#define HAVE_H5FREE_MEMORY 1
#define HAVE_H5PSET_DEFLATE 1
#define HAVE_H5RESIZE_MEMORY 1
#define HAVE_HDF5_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIBGEN_H 1
#define HAVE_LIBM 1
#define HAVE_LOCALE_H 1
#define HAVE_LONG_LONG_INT 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMORY_H 1
#define HAVE_MKSTEMP 1
#define HAVE_MKTEMP 1
#define HAVE_MMAP 1
#define HAVE_RANDOM 1
#define HAVE_SIZE_T 1
#define HAVE_SNPRINTF 1
#define HAVE_SSIZE_T 1
#define HAVE_STDARG_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRLCAT 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_ST_BLKSIZE 1
#define HAVE_SYSCONF 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H 1
#define HAVE_UINT 1
#define HAVE_UINT64_T 1
#define HAVE_UINTPTR_T 1
#define HAVE_UNISTD_H 1
#define HAVE_UNSIGNED_LONG_LONG_INT 1
#define HAVE_USHORT 1
#define HDF5_SUPPORTS_PAR_FILTERS 1
#define HDF5_UTF8_PATHS 1
#define LT_OBJDIR ".libs/"
#define MAX_DEFAULT_CACHE_SIZE 67108864
#define NCIO_MINBLOCKSIZE 256
#define NCPROPERTIES_EXTRA ""
#define NC_DISPATCH_VERSION 3
#define SIZEOF_DOUBLE 8
#define SIZEOF_FLOAT 4
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_OFF_T 8
#define SIZEOF_SHORT 2
#define SIZEOF_SIZE_T 8
#define SIZEOF_SSIZE_T 8
#define SIZEOF_UINT 4
#define SIZEOF_UNSIGNED_CHAR 1
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_USHORT 2
#define SIZEOF_VOIDP 8
#define STDC_HEADERS 1
#define TEMP_LARGE "."
#define USE_DAP 1
#define USE_EXTREME_NUMBERS 1
#define USE_HDF5 1
#define USE_NETCDF4 1
#define USE_NETCDF_2 1
#endif

#include "ncconfigure.h"

#endif
