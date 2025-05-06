{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_ogr_sqlite_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqliteutility.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqlitedriver.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqlitedatasource.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/test_load_virtual_ogr.c",
				"../gdal/ogr/ogrsf_frmts/sqlite/my_test_sqlite3_ext.c",
				"../gdal/ogr/ogrsf_frmts/sqlite/rasterlite2.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqlitelayer.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqlitevfs.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqliteselectlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqliteviewlayer.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqlitetablelayer.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqliteapiroutines.c",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqliteexecutesql.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqlitevirtualogr.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/sqlite_rtree_bulk_load/wrapper.cpp",
				"../gdal/ogr/ogrsf_frmts/sqlite/ogrsqlitesinglefeaturelayer.cpp"
			],
			"include_dirs": [
				"../gdal/ogr/ogrsf_frmts/sqlite",
				"../gdal/ogr/ogrsf_frmts/sqlite/sqlite_rtree_bulk_load"
			]
		}
	]
}
