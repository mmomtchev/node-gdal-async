{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_gpx_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/gpx/ogrgpxdriver.cpp",
				"../gdal/ogr/ogrsf_frmts/gpx/ogrgpxlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/gpx/ogrgpxdatasource.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/gpx"
			]
		}
	]
}
