{
	"includes": [
		"../../common.gypi"
	],
	"targets": [
		{
			"target_name": "libaec",
			"type": "static_library",
			"sources": [
        "./libaec/src/graec.c",
        "./libaec/src/encode_accessors.c",
        "./libaec/src/decode.c",
        "./libaec/src/vector.c",
        "./libaec/src/encode.c",
        "./libaec/src/sz_compat.c"
			],
			"include_dirs": [
				"./config",
        "./libaec/src",
        "./libaec/include"
			],
			"conditions": [
				["OS == 'win'", {
					"defines": [
						"_WINDOWS"
					],
				}],
				["OS == 'linux'", {
					"defines": [
						"_GNU_SOURCE=1"
					]
				}],
				["OS == 'mac'", {
					"defines": [
						"DARWIN"
					]
				}]
			],
			"direct_dependent_settings": {
				"include_dirs": [
					"./libaec/config",
					"./libaec/include",
				]
			}
		}
	]
}
