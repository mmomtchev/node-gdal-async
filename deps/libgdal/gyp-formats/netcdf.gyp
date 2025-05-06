{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_netcdf_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/netcdf/netcdfsg.cpp",
				"../gdal/frmts/netcdf/netcdflayersg.cpp",
				"../gdal/frmts/netcdf/netcdfmultidim.cpp",
				"../gdal/frmts/netcdf/netcdfvirtual.cpp",
				"../gdal/frmts/netcdf/netcdflayer.cpp",
				"../gdal/frmts/netcdf/netcdfdrivercore.cpp",
				"../gdal/frmts/netcdf/netcdfsgwriterutil.cpp",
				"../gdal/frmts/netcdf/netcdfwriterconfig.cpp",
				"../gdal/frmts/netcdf/netcdfdataset.cpp",
				"../gdal/frmts/netcdf/netcdf_sentinel3_sral_mwr.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/netcdf"
			]
		}
	]
}
