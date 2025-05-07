{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_aigrid_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/aigrid/aigopen.c",
				"../gdal/frmts/aigrid/aigdataset.cpp",
				"../gdal/frmts/aigrid/aitest.c",
				"../gdal/frmts/aigrid/gridlib.c",
				"../gdal/frmts/aigrid/aigccitt.c"
			],
			"include_dirs": [
				"../gdal/frmts/aigrid"
			]
		}
	]
}
