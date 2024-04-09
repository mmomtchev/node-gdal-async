{
	"variables": {
    "runtime%": "node",
		"deps_dir": "./deps"
	},
	"target_defaults": {
		"default_configuration": "Release",
		"cflags_cc!": ["-fno-rtti", "-fno-exceptions"],
		"cflags!": ["-fno-rtti", "-fno-exceptions"],
		"variables": {
			"debug_extra_ccflags_cc%": [],
			"debug_extra_ldflags%" : [],
      "debug_extra_VCCLCompilerTool%": [],
      "debug_extra_VCLinkerTool%": []
		},
		"defines": [
			"NOGDI=1",
      "HAVE_LIBZ=1"
		],
		"xcode_settings": {
			"GCC_ENABLE_CPP_RTTI": "YES",
			"GCC_ENABLE_CPP_EXCEPTIONS": "YES",
      "CLANG_CXX_LIBRARY": "libc++",
      "MACOSX_DEPLOYMENT_TARGET": "10.7",
			"OTHER_CFLAGS": [
				"-Wno-deprecated-register",
				"-Wno-unused-const-variable"
			],
			"OTHER_CPLUSPLUSFLAGS": [
				"-Wno-deprecated-register",
				"-Wno-unused-const-variable",
				"-frtti",
				"-fexceptions"
			]
		},
    "msvs_settings": {
      "VCCLCompilerTool": {
        #"Optimization": 0, # 0:/Od disable, 1:/O1 min size, 2:/O2 max speed, 3:/Ox full optimization
        #"InlineFunctionExpansion": 0, #0:/Ob0: disable, 1:/Ob1 inline only marked funtions, 2:/Ob2 inline anything eligible
        "AdditionalOptions": [
          "/MP", # compile across multiple CPUs
          "/GR", # force RTTI on (see https://github.com/nodejs/node-gyp/issues/2412)
          "/EHsc" # same for ExceptionHandling
        ],
        "ExceptionHandling": 1,
        "RuntimeTypeInfo": "true"
      }
    },
		"conditions": [
        ["runtime == 'node'", {
          "defines": [
          ]
        }],
			["OS == 'win'", {
				"defines": [
          "NOMINMAX",
          "WIN32",
          "CURL_STATICLIB",
          "PROJ_DLL=",
          "OPJ_EXPORTS"
        ],
				"libraries": [
					"secur32.lib",
					"odbccp32.lib",
					"odbc32.lib",
					"crypt32.lib",
					"ws2_32.lib",
					"advapi32.lib",
					"wbemuuid.lib",
          "Shlwapi.lib"
				],
			}],
			["OS == 'mac'", {
				"libraries": [ "-framework Security" ],
				"defines": [ "DARWIN" ]
			}],
			["OS == 'linux'", {
  			"defines": [ "LINUX" ],
        "ldflags": [
          "-Wl,--exclude-libs,ALL"
        ]
			}]
		],
		"configurations": {
			"Debug": {
				"cflags_cc!": [ "-O3", "-Os" ],
				"cflags_cc": [ "<@(debug_extra_ccflags_cc)" ],
				"ldflags": [ "<@(debug_extra_ldflags)" ],
				"defines": [ "DEBUG" ],
				"defines!": [ "NDEBUG" ],
				"xcode_settings": {
					"GCC_OPTIMIZATION_LEVEL": "0",
					"GCC_GENERATE_DEBUGGING_SYMBOLS": "YES",
					"OTHER_CPLUSPLUSFLAGS": [ "<@(debug_extra_ccflags_cc)" ],
					"OTHER_LDFLAGS": [ "<@(debug_extra_ldflags)" ]
				}
      },
			"Release": {
				"defines": [ "NDEBUG" ],
				"defines!": [ "DEBUG" ],
				"xcode_settings": {
					"GCC_OPTIMIZATION_LEVEL": "s",
					"GCC_GENERATE_DEBUGGING_SYMBOLS": "NO",
					"DEAD_CODE_STRIPPING": "YES",
					"GCC_INLINES_ARE_PRIVATE_EXTERN": "YES"
				},
				"ldflags": [
					"-Wl,-s"
				],
				"msvs_settings": {
					"VCCLCompilerTool": {
						"DebugInformationFormat": "0",
					},
					"VCLinkerTool": {
						"GenerateDebugInformation": "false",
					}
				}
			}
		}
	}
}
