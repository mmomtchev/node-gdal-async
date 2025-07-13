{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_wms_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/wms/wmsutils.cpp",
				"../gdal/frmts/wms/gdalwmscache.cpp",
				"../gdal/frmts/wms/minidriver_arcgis_server.cpp",
				"../gdal/frmts/wms/minidriver_iip.cpp",
				"../gdal/frmts/wms/minidriver_ogcapimaps.cpp",
				"../gdal/frmts/wms/minidriver_tiled_wms.cpp",
				"../gdal/frmts/wms/wmsmetadataset.cpp",
				"../gdal/frmts/wms/minidriver_ogcapicoverage.cpp",
				"../gdal/frmts/wms/gdalhttp.cpp",
				"../gdal/frmts/wms/minidriver_mrf.cpp",
				"../gdal/frmts/wms/minidriver_wms.cpp",
				"../gdal/frmts/wms/wmsdrivercore.cpp",
				"../gdal/frmts/wms/wmsdriver.cpp",
				"../gdal/frmts/wms/gdalwmsdataset.cpp",
				"../gdal/frmts/wms/minidriver_worldwind.cpp",
				"../gdal/frmts/wms/minidriver.cpp",
				"../gdal/frmts/wms/minidriver_tileservice.cpp",
				"../gdal/frmts/wms/gdalwmsrasterband.cpp",
				"../gdal/frmts/wms/minidriver_tms.cpp",
				"../gdal/frmts/wms/minidriver_virtualearth.cpp",
				"../gdal/frmts/wms/minidriver_iiifimage.cpp",
			],
			"include_dirs": [
				"../gdal/frmts/wms"
			]
		}
	]
}
