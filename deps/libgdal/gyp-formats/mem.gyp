{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_mem_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/mem/memdataset.cpp",
				"../gdal/frmts/mem/ogrmemlayer.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/mem"
			]
		}
	]
}
