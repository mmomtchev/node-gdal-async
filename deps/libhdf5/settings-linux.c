/* Generated automatically by H5make_libsettings -- do not edit */



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Created:		May 26, 2021
 *			Momtchil Momtchev <mmom@mmom-workstation>
 *
 * Purpose:		This machine-generated source code contains
 *			information about the library build configuration
 *
 * Modifications:
 *
 *	DO NOT MAKE MODIFICATIONS TO THIS FILE!
 *	It was generated by code in `H5make_libsettings.c'.
 *
 *-------------------------------------------------------------------------
 */

char H5libhdf5_settings[]=
	"	    SUMMARY OF THE HDF5 CONFIGURATION\n"
	"	    =================================\n"
	"\n"
	"General Information:\n"
	"-------------------\n"
	"                   HDF5 Version: 1.12.0\n"
	"                  Configured on: Wed May 26 10:32:35 CEST 2021\n"
	"                  Configured by: mmom@mmom-workstation\n"
	"                    Host system: x86_64-unknown-linux-gnu\n"
	"              Uname information: Linux mmom-workstation 5.8.0-53-generic #60~20.04.1-Ubuntu SMP Thu May 6 09:52:46 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux\n"
	"                       Byte sex: little-endian\n"
	"             Installation point: /home/mmom/src/hdf5-hdf5-1_12_0/hdf5\n"
	"\n"
	"Compiling Options:\n"
	"------------------\n"
	"                     Build Mode: production\n"
	"              Debugging Symbols: no\n"
	"                        Asserts: no\n"
	"                      Profiling: no\n"
	"             Optimization Level: high\n"
	"\n"
	"Linking Options:\n"
	"----------------\n"
	"                      Libraries: static\n"
	"  Statically Linked Executables: \n"
	"                        LDFLAGS: \n"
	"                     H5_LDFLAGS: \n"
	"                     AM_LDFLAGS: \n"
	"                Extra libraries: -lpthread -lz -ldl -lm \n"
	"                       Archiver: ar\n"
	"                       AR_FLAGS: cr\n"
	"                         Ranlib: ranlib\n"
	"\n"
	"Languages:\n"
	"----------\n"
	"                              C: yes\n"
	"                     C Compiler: /usr/bin/gcc\n"
	"                       CPPFLAGS: \n"
	"                    H5_CPPFLAGS: -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L   -DNDEBUG -UH5_DEBUG_API\n"
	"                    AM_CPPFLAGS: \n"
	"                        C Flags: \n"
	"                     H5 C Flags:  -std=c99  -Wall -Wcast-qual -Wconversion -Wextra -Wfloat-equal -Wformat=2 -Winit-self -Winvalid-pch -Wmissing-include-dirs -Wno-c++-compat -Wno-format-nonliteral -Wshadow -Wundef -Wwrite-strings -pedantic -Wbad-function-cast -Wcast-align -Wdeclaration-after-statement -Wdisabled-optimization -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wold-style-definition -Wpacked -Wredundant-decls -Wstrict-prototypes -Wswitch-enum -Wswitch-default -Wunused-macros -Wunsafe-loop-optimizations -Wlogical-op -Wlarger-than=2560 -Wsync-nand -Wframe-larger-than=16384 -Wpacked-bitfield-compat -Wstrict-overflow=5 -Wjump-misses-init -Wunsuffixed-float-constants -Wdouble-promotion -Wtrampolines -Wstack-usage=8192 -Wvector-operation-performance -Wdate-time -Warray-bounds=2 -Wc99-c11-compat -Wnull-dereference -Wunused-const-variable -Wduplicated-cond -Whsa -Wnormalized -Walloc-zero -Walloca -Wduplicated-branches -Wformat-overflow=2 -Wformat-truncation=2 -Wimplicit-fallthrough=5 -Wrestrict -Wattribute-alias -Wcast-align=strict -Wshift-overflow=2 -Wattribute-alias=2 -Wmissing-profile -fstdarg-opt -s -Wno-aggregate-return -Wno-inline -Wno-missing-format-attribute -Wno-missing-noreturn -Wno-suggest-attribute=const -Wno-suggest-attribute=pure -Wno-suggest-attribute=noreturn -Wno-suggest-attribute=format -Wno-suggest-attribute=cold -Wno-suggest-attribute=malloc -O3\n"
	"                     AM C Flags: \n"
	"               Shared C Library: no\n"
	"               Static C Library: yes\n"
	"\n"
	"\n"
	"                        Fortran: no\n"
	"\n"
	"                            C++: yes\n"
	"                   C++ Compiler: /usr/bin/g++\n"
	"                      C++ Flags: \n"
	"                   H5 C++ Flags:   -pedantic -Wall -W -Wundef -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wredundant-decls -Winline -Wsign-promo -Woverloaded-virtual -Wold-style-cast -Weffc++ -Wreorder -Wnon-virtual-dtor -Wctor-dtor-privacy -Wabi -finline-functions -s -O\n"
	"                   AM C++ Flags: \n"
	"             Shared C++ Library: no\n"
	"             Static C++ Library: yes\n"
	"\n"
	"                           Java: no\n"
	"\n"
	"\n"
	"Features:\n"
	"---------\n"
	"                   Parallel HDF5: no\n"
	"Parallel Filtered Dataset Writes: no\n"
	"              Large Parallel I/O: no\n"
	"              High-level library: yes\n"
	"                Build HDF5 Tests: no\n"
	"                Build HDF5 Tools: no\n"
	"                    Threadsafety: yes\n"
	"             Default API mapping: v18\n"
	"  With deprecated public symbols: yes\n"
	"          I/O filters (external): deflate(zlib)\n"
	"                             MPE: no\n"
	"                   Map (H5M) API: no\n"
	"                      Direct VFD: no\n"
	"              (Read-Only) S3 VFD: no\n"
	"            (Read-Only) HDFS VFD: no\n"
	"                         dmalloc: no\n"
	"  Packages w/ extra debug output: none\n"
	"                     API tracing: no\n"
	"            Using memory checker: no\n"
	" Memory allocation sanity checks: no\n"
	"          Function stack tracing: no\n"
	"       Strict file format checks: no\n"
	"    Optimization instrumentation: no\n"
;

