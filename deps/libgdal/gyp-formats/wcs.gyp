{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_wcs_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/wcs/wcsdrivercore.cpp",
				"../gdal/frmts/wcs/wcsdataset201.cpp",
				"../gdal/frmts/wcs/wcsrasterband.cpp",
				"../gdal/frmts/wcs/wcsdataset100.cpp",
				"../gdal/frmts/wcs/gmlcoverage.cpp",
				"../gdal/frmts/wcs/wcsdataset110.cpp",
				"../gdal/frmts/wcs/wcsdataset.cpp",
				"../gdal/frmts/wcs/wcsutils.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/wcs",
				"../gdal/frmts/gcore",
				"../gdal/ogr",
				"../gdal/ogr/ogrsf_frmts/gml",
				"../gdal/ogr/ogrsf_frmts/gmlutils"
			]
		}
	]
}
