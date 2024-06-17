{
	"includes": [
		"../../common.gypi"
	],
	"targets": [
		{
			"target_name": "libopenjpeg",
			"type": "static_library",
			"cflags": [
				"-Wno-unused-variable",
				"-Wno-unused-but-set-variable",
				"-Wno-unused-function",
				"-Wno-missing-braces",
				"-Wno-int-in-bool-context",
				"-Wno-stringop-truncation",
				"-Wno-implicit-fallthrough",
				"-Wno-switch"
			],
			"sources": [
				"openjpeg/src/lib/openjp2/thread.c",
				"openjpeg/src/lib/openjp2/bio.c",
				"openjpeg/src/lib/openjp2/cio.c",
				"openjpeg/src/lib/openjp2/dwt.c",
				"openjpeg/src/lib/openjp2/event.c",
				"openjpeg/src/lib/openjp2/image.c",
				"openjpeg/src/lib/openjp2/invert.c",
				"openjpeg/src/lib/openjp2/j2k.c",
				"openjpeg/src/lib/openjp2/jp2.c",
				"openjpeg/src/lib/openjp2/mct.c",
				"openjpeg/src/lib/openjp2/mqc.c",
				"openjpeg/src/lib/openjp2/openjpeg.c",
				"openjpeg/src/lib/openjp2/opj_clock.c",
				"openjpeg/src/lib/openjp2/pi.c",
				"openjpeg/src/lib/openjp2/t1.c",
				"openjpeg/src/lib/openjp2/t2.c",
				"openjpeg/src/lib/openjp2/tcd.c",
				"openjpeg/src/lib/openjp2/tgt.c",
				"openjpeg/src/lib/openjp2/function_list.c",
				"openjpeg/src/lib/openjp2/opj_malloc.c",
				"openjpeg/src/lib/openjp2/sparse_array.c"
			],
			"defines": [
				"OPJ_STATIC=1"
			],
			"conditions": [
				["OS == 'win'", {
					"defines": [
						"OPJ_HAVE_MALLOC_H"
					]
				}],
				["OS == 'linux'", {
					"defines": [
						"OPJ_HAVE_MALLOC_H",
						"OPJ_HAVE_MEMALIGN",
						"OPJ_HAVE_POSIX_MEMALIGN"
					],
				}],
				["OS == 'mac'", {
					"defines": [
						"OPJ_HAVE_MEMALIGN",
						"OPJ_HAVE_POSIX_MEMALIGN"
					],
				}]
			],
			"direct_dependent_settings": {
				"include_dirs": [
					"openjpeg/src/lib/openjp2"
				]
			}
		}
	]
}
