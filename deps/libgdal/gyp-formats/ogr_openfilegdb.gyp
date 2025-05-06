{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_openfilegdb_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/openfilegdb/ogropenfilegdblayer.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/filegdbtable_freelist.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/ogropenfilegdbdatasource_write.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/filegdbindex_write.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/filegdbtable_write.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/ogropenfilegdblayer_write.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/filegdbtable.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/filegdbtable_write_fields.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/ogropenfilegdbdatasource.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/ogropenfilegdbdriver.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/filegdbindex.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/test_ofgdb_write.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/gdalopenfilegdbrasterband.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/ogropenfilegdb_generate_uuid.cpp",
				"../gdal/ogr/ogrsf_frmts/openfilegdb/ogropenfilegdbdrivercore.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/openfilegdb"
			]
		}
	]
}
