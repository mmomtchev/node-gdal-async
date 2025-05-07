{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_gsg_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/gsg/gs7bgdataset.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/gsg"
			]
		}
	]
}
