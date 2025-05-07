{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_vrt_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/vrt/vrtsourcedrasterband.cpp",
				"../gdal/frmts/vrt/vrtrasterband.cpp",
				"../gdal/frmts/vrt/pixelfunctions.cpp",
				"../gdal/frmts/vrt/vrtsources.cpp",
				"../gdal/frmts/vrt/vrtdataset.cpp",
				"../gdal/frmts/vrt/vrtprocesseddatasetfunctions.cpp",
				"../gdal/frmts/vrt/vrtfilters.cpp",
				"../gdal/frmts/vrt/vrtpansharpened.cpp",
				"../gdal/frmts/vrt/vrtprocesseddataset.cpp",
				"../gdal/frmts/vrt/vrtwarped.cpp",
				"../gdal/frmts/vrt/vrtreclassifier.cpp",
				"../gdal/frmts/vrt/vrtmultidim.cpp",
				"../gdal/frmts/vrt/vrtdriver.cpp",
				"../gdal/frmts/vrt/vrtderivedrasterband.cpp",
				"../gdal/frmts/vrt/vrtrawrasterband.cpp",
				"../gdal/frmts/vrt/vrtexpression_exprtk.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/vrt",
				"../gdal/frmts/raw",
				"../../exprtk"
			],
			"defines": [
				"GDAL_VRT_ENABLE_EXPRTK=1"
			]
		}
	]
}
