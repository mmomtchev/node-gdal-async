{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_openjpeg_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/openjpeg/openjpegdataset.cpp",
				"../gdal/frmts/openjpeg/openjpegdrivercore.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/openjpeg",
				"../gdal/frmts/opjlike"
			]
		}
	]
}
