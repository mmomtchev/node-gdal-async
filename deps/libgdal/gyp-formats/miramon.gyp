{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_miramon_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/miramon/miramon_dataset.cpp",
				"../gdal/frmts/miramon/miramon_rel.cpp",
				"../gdal/frmts/miramon/miramon_rasterband.cpp",
				"../gdal/frmts/miramon/miramon_band.cpp",
				"../gdal/frmts/miramon/miramon_palettes.cpp",
				"../gdal/frmts/miramon_common/mm_gdal_functions.c"
			],
			"include_dirs": [
				"../gdal/frmts/miramon"
			]
		}
	]
}
