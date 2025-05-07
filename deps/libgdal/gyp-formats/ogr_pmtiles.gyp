{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_pmtiles_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/pmtiles/ogrpmtilesdriver.cpp",
				"../gdal/ogr/ogrsf_frmts/pmtiles/vsipmtiles.cpp",
				"../gdal/ogr/ogrsf_frmts/pmtiles/ogrpmtilesfrommbtiles.cpp",
				"../gdal/ogr/ogrsf_frmts/pmtiles/ogrpmtilesvectorlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/pmtiles/ogrpmtilesdataset.cpp",
				"../gdal/ogr/ogrsf_frmts/pmtiles/ogrpmtileswriterdataset.cpp",
				"../gdal/ogr/ogrsf_frmts/pmtiles/ogrpmtilestileiterator.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/pmtiles",
				"../gdal/ogr/ogrsf_frmts/pmtiles/pmtiles",
				"../gdal/ogr/ogrsf_frmts/mvt"
			]
		}
	]
}
