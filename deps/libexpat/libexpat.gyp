{
	"includes": [
		"../../common.gypi"
	],
	"variables": {
		"endianness": "<!(<(python) -c \"import sys;print(sys.byteorder.upper())\")",
	},
	"targets": [
		{
			"target_name": "libexpat",
			"type": "static_library",
			"sources": [
				'<!@(<(python) ../glob-files.py "expat/lib/*.c")'
			],
			"include_dirs": [
				"./expat/lib"
			],
			"conditions": [
				["OS == 'win'", {
					"include_dirs": ["./arch/win"]
				}, {
					"include_dirs": ["./arch/unix"]
				}]
			],
			"defines": [
				"XML_STATIC=1",
				"XML_POOR_ENTROPY=1",
				"HAVE_EXPAT_CONFIG_H=1",
				"CPU_<(endianness)_ENDIAN=1"
			],
			"direct_dependent_settings": {
				"include_dirs": [
					"./expat/lib",
				],
				"defines": [
					"XML_STATIC=1",
					"XML_POOR_ENTROPY=1",
					"HAVE_EXPAT_CONFIG_H=1",
				],
				"conditions": [
					["OS == 'win'", {
						"include_dirs": ["./arch/win"]
					}, {
						"include_dirs": ["./arch/unix"]
					}]
				]
			}
		}
	]
}
