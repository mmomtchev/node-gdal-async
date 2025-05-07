{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_libertiff_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/libertiff/libertiffdataset.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/libertiff",
        "../gdal/third_party/libertiff",
        "../gdal/frmts/gtiff/libtiff"
			]
		}
	]
}
