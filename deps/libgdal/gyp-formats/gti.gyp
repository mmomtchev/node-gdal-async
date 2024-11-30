{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_gti_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/gti/gdaltileindexdataset.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/gti",
				"../gdal/frmts/gti/data"
			]
		}
	]
}
