{
	"includes": [
		"../../common.gypi",
	],
	"targets": [
		{
			"includes": [ "../../cpp_17.gypi" ],
			"target_name": "libproj",
			"type": "static_library",
			"cflags_cc": ["-Wno-attributes"],
			"cflags": ["-Wno-attributes"],
			"sources": [
				'<!@(<(python) ../glob-files.py "proj/src/*.c")',
				'<!@(<(python) ../glob-files.py "proj/src/*.cpp")',
				'<!@(<(python) ../glob-files.py "proj/src/*/*.cpp")',
				'<!@(<(python) ../glob-files.py "proj/src/*/*/*.cpp")'
			],
			"include_dirs": [
				"./proj/src",
				"./proj/include"
			],
			"conditions": [
				["OS == 'win'", {
					"include_dirs": ["./arch/win/src"]
				}, {
					"include_dirs": ["./arch/unix/src"]
				}]
			],
			"dependencies": [
				"../libsqlite/libsqlite.gyp:libsqlite"
			],
			"direct_dependent_settings": {
				"include_dirs": [
					"./proj/src"
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
