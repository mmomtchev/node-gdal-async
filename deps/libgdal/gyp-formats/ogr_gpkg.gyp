{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_gpkg_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/gpkg/gdalgeopackagerasterband.cpp",
				"../gdal/ogr/ogrsf_frmts/gpkg/ogrgeopackagelayer.cpp",
				"../gdal/ogr/ogrsf_frmts/gpkg/ogrgeopackagetablelayer.cpp",
				"../gdal/ogr/ogrsf_frmts/gpkg/ogrgeopackagedriver.cpp",
				"../gdal/ogr/ogrsf_frmts/gpkg/ogrgeopackageselectlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/gpkg/ogrgeopackagedatasource.cpp",
				"../gdal/ogr/ogrsf_frmts/gpkg/ogrgeopackageutility.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/sqlite",
				"../gdal/ogr/ogrsf_frmts/gpkg"
			]
		}
	]
}
