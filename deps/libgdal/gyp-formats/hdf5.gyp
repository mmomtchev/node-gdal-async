{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_hdf5_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/hdf5/s102dataset.cpp",
				"../gdal/frmts/hdf5/s100.cpp",
				"../gdal/frmts/hdf5/hdf5drivercore.cpp",
				"../gdal/frmts/hdf5/bagdataset.cpp",
				"../gdal/frmts/hdf5/gh5_convenience.cpp",
				"../gdal/frmts/hdf5/s111dataset.cpp",
				"../gdal/frmts/hdf5/hdf5dataset.cpp",
				"../gdal/frmts/hdf5/hdf5imagedataset.cpp",
				"../gdal/frmts/hdf5/hdf5eosparser.cpp",
				"../gdal/frmts/hdf5/rat.cpp",
				"../gdal/frmts/hdf5/iso19115_srs.cpp",
				"../gdal/frmts/hdf5/hdf5multidim.cpp",
				"../gdal/frmts/hdf5/s104dataset.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/hdf5"
			]
		}
	]
}
