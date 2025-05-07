{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_gmt_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/gmt/ogrgmtlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/gmt/ogrgmtdatasource.cpp",
				"../gdal/ogr/ogrsf_frmts/gmt/ogrgmtdriver.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/gmt"
			]
		}
	]
}
