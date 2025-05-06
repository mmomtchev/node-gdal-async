cmake_minimum_required(VERSION 3.13...3.19)
project(libaec LANGUAGES C VERSION 1.1.3)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(CTest)

include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)

# Check for __builtin_clzll for faster decoding
include(CheckCSourceCompiles)
check_c_source_compiles(
  "int main(void)\n{return __builtin_clzll(1LL);}"
  HAVE_DECL___BUILTIN_CLZLL)

if(NOT HAVE_DECL___BUILTIN_CLZLL)
  # With MSVC we can use _BitScanReverse64
  check_c_source_compiles(
    "int main(void){unsigned long foo; unsigned __int64 bar=1LL;
return _BitScanReverse64(&foo, bar);}"
    HAVE_BSR64)
endif()

include(CheckSymbolExists)
check_symbol_exists(snprintf "stdio.h" HAVE_SNPRINTF)
if(NOT HAVE_SNPRINTF)
  check_symbol_exists(_snprintf "stdio.h" HAVE__SNPRINTF)
  check_symbol_exists(_snprintf_s "stdio.h" HAVE__SNPRINTF_S)
endif()

# Communicate findings to code. Has to be compatible with autoconf's config.h.
configure_file(
  "cmake/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/config.h")

# Create libaec.h with current version information
configure_file(
  "include/libaec.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/libaec.h")

add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

option(AEC_FUZZING "Enable build of fuzzing target" OFF)
if(AEC_FUZZING)
  enable_language(CXX)

  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "You need to build with Clang for fuzzing to work")
  endif()

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6.0.0")
    message(FATAL_ERROR "You need Clang ≥ 6.0.0")
  endif()

  add_subdirectory(fuzzing)
  set(FUZZ_TARGET_SAN_FLAGS -fsanitize=fuzzer-no-link,address,undefined)
  target_compile_options(aec PUBLIC -g -O1 ${FUZZ_TARGET_SAN_FLAGS})
  target_link_options(aec PUBLIC ${FUZZ_TARGET_SAN_FLAGS})
endif()

# Install the cmake files.
set(libaec_CONFIG_IN  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libaec-config.cmake.in)
set(libaec_CONFIG_OUT ${CMAKE_CURRENT_BINARY_DIR}/cmake/libaec-config.cmake)
configure_file(${libaec_CONFIG_IN} ${libaec_CONFIG_OUT} @ONLY)
set(libaec_CONFIG_VERSION_IN  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libaec-config-version.cmake.in)
set(libaec_CONFIG_VERSION_OUT ${CMAKE_CURRENT_BINARY_DIR}/cmake/libaec-config-version.cmake)
configure_file(${libaec_CONFIG_VERSION_IN} ${libaec_CONFIG_VERSION_OUT} @ONLY)
install(FILES ${libaec_CONFIG_OUT}
        DESTINATION cmake)
install(FILES ${libaec_CONFIG_VERSION_OUT}
        DESTINATION cmake)

# Cpack configuration mainly for Windows installer
set(CPACK_PACKAGE_NAME "libaec")
set(CPACK_PACKAGE_VENDOR "Deutsches Klimarechenzentrum GmbH")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "libaec - Adaptive Entropy Coding library")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "libaec")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

if(WIN32)
  set(CPACK_GENERATOR "WIX")
  set(CPACK_WIX_UPGRADE_GUID "E58A21F6-BB99-46B5-8AD8-6114E4CCA0A6")
endif()

set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /.*build.*
    /*.cache
    .*~
    .*\#.*\#
    \..*\#.*)

include(CPack)
