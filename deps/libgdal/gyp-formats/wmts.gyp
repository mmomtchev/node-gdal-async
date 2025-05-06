{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_wmts_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/wmts/wmtsdrivercore.cpp",
				"../gdal/frmts/wmts/wmtsdataset.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/wmts"
			]
		}
	]
}
