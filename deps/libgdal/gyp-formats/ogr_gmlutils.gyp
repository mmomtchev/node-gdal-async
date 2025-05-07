{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_gmlutils_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/gmlutils/gmlfeature.cpp",
				"../gdal/ogr/ogrsf_frmts/gmlutils/gmlfeatureclass.cpp",
				"../gdal/ogr/ogrsf_frmts/gmlutils/gmlpropertydefn.cpp",
				"../gdal/ogr/ogrsf_frmts/gmlutils/parsexsd.cpp",
				"../gdal/ogr/ogrsf_frmts/gmlutils/ogrwfsfilter.cpp",
				"../gdal/ogr/ogrsf_frmts/gmlutils/gmlutils.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/gmlutils"
			]
		}
	]
}
