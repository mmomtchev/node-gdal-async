{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_mbtiles_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/mbtiles/mbtilesdataset.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/sqlite",
				"../gdal/ogr/ogrsf_frmts/gpkg",
				"../gdal/ogr/ogrsf_frmts/mvt",
				"../gdal/frmts/mbtiles"
			]
		}
	]
}
