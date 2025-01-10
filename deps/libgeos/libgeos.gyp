{
	"includes": [
		"../../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgeos",
			"type": "static_library",
			"sources": [
				# <(python) 2 doesnt have recursive globs! deepest is 4 levels, so do it hacky
				'<!@(<(python) ../glob-files.py "geos/capi/*.cpp")',
				'<!@(<(python) ../glob-files.py "geos/src/*.cpp")',
				'<!@(<(python) ../glob-files.py "geos/src/*/*.cpp")',
				'<!@(<(python) ../glob-files.py "geos/src/*/*/*.cpp")',
				'<!@(<(python) ../glob-files.py "geos/src/*/*/*/*.cpp")',
				'<!@(<(python) ../glob-files.py "geos/src/deps/*/*.c")'
			],
			"include_dirs": [
				"./include",
				"./geos/src/deps",
				"./geos/include",
				"./geos/capi"
			],
			"defines": [
				"USE_UNSTABLE_GEOS_CPP_API"
			],
			"conditions": [
				["OS == 'win'", {
					"include_dirs": ["./arch/win"],
					# not every file that needs to requires arch/win, so do a couple extra defines
					"defines": ["TTMATH_NOASM" ],
				}, {
					"include_dirs": ["./arch/unix"]
				}]
			],
			"direct_dependent_settings": {
				"include_dirs": [
					"./include",
					"./geos/include",
					"./geos/capi"
				],
				"conditions": [
					["OS == 'win'", {
						"include_dirs": ["./arch/win"]
					}, {
						"include_dirs": ["./arch/unix"]
					}]
				],
			}
		}
	]
}
