{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_iso8211_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/iso8211/ddffield.cpp",
				"../gdal/frmts/iso8211/8211dump.cpp",
				"../gdal/frmts/iso8211/ddfrecord.cpp",
				"../gdal/frmts/iso8211/ddfsubfielddefn.cpp",
				"../gdal/frmts/iso8211/8211view.cpp",
				"../gdal/frmts/iso8211/ddfutils.cpp",
				"../gdal/frmts/iso8211/8211createfromxml.cpp",
				"../gdal/frmts/iso8211/ddfmodule.cpp",
				"../gdal/frmts/iso8211/ddffielddefn.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/iso8211"
			]
		}
	]
}
