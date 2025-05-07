{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_carto_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/carto/ogrcartodriver.cpp",
				"../gdal/ogr/ogrsf_frmts/carto/ogrcartotablelayer.cpp",
				"../gdal/ogr/ogrsf_frmts/carto/ogrcartolayer.cpp",
				"../gdal/ogr/ogrsf_frmts/carto/ogrcartoresultlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/carto/ogrcartodatasource.cpp",
				"../gdal/ogr/ogrsf_frmts/carto/ogrcartodrivercore.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/carto",
				"../gdal/ogr/ogrsf_frmts/pgdump",
				"../gdal/ogr/ogrsf_frmts/geojson"
			]
		}
	]
}
