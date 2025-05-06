{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_envisat_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/envisat/envisatdataset.cpp",
				"../gdal/frmts/envisat/records.c",
				"../gdal/frmts/envisat/EnvisatFile.c",
				"../gdal/frmts/envisat/adsrange.cpp",
				"../gdal/frmts/envisat/dumpgeo.c",
				"../gdal/frmts/envisat/envisat_dump.c",
				"../gdal/frmts/envisat/unwrapgcps.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/envisat"
			]
		}
	]
}
