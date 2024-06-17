{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_tiledb_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/tiledb/tiledbmultidimattributeholder.cpp",
				"../gdal/frmts/tiledb/tiledbmultidimarray.cpp",
				"../gdal/frmts/tiledb/tiledbmultidimattribute.cpp",
				"../gdal/frmts/tiledb/tiledbdense.cpp",
				"../gdal/frmts/tiledb/tiledbsparse.cpp",
				"../gdal/frmts/tiledb/tiledbmultidimgroup.cpp",
				"../gdal/frmts/tiledb/tiledbmultidim.cpp",
				"../gdal/frmts/tiledb/tiledbcommon.cpp",
				"../gdal/frmts/tiledb/tiledbdrivercore.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/tiledb"
			]
		}
	]
}
