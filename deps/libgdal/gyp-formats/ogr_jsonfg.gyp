{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_jsonfg_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/jsonfg/ogrjsonfgwritelayer.cpp",
				"../gdal/ogr/ogrsf_frmts/jsonfg/ogrjsonfgdataset.cpp",
				"../gdal/ogr/ogrsf_frmts/jsonfg/ogrjsonfgstreamingparser.cpp",
				"../gdal/ogr/ogrsf_frmts/jsonfg/ogrjsonfgstreamedlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/jsonfg/ogrjsonfgreader.cpp",
				"../gdal/ogr/ogrsf_frmts/jsonfg/ogrjsonfgmemlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/jsonfg/ogrjsonfgdriver.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/jsonfg"
			]
		}
	]
}
