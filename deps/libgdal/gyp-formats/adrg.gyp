{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_adrg_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/adrg/srpdataset.cpp",
				"../gdal/frmts/adrg/adrgdataset.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/adrg",
				"../gdal/frmts/iso8211"
			]
		}
	]
}
