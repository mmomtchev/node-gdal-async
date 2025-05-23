{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_zlib_frmt",
			"type": "static_library",
			"sources": [
				"../gdal/frmts/zlib/inflate.c",
				"../gdal/frmts/zlib/compress.c",
				"../gdal/frmts/zlib/deflate.c",
				"../gdal/frmts/zlib/contrib/infback9/inftree9.c",
				"../gdal/frmts/zlib/contrib/infback9/minified_zutil.c",
				"../gdal/frmts/zlib/contrib/infback9/infback9.c",
				"../gdal/frmts/zlib/crc32.c",
				"../gdal/frmts/zlib/infback.c",
				"../gdal/frmts/zlib/zutil.c",
				"../gdal/frmts/zlib/inftrees.c",
				"../gdal/frmts/zlib/uncompr.c",
				"../gdal/frmts/zlib/trees.c",
				"../gdal/frmts/zlib/inffast.c",
				"../gdal/frmts/zlib/adler32.c"
			],
			"include_dirs": [
				"../gdal/frmts/zlib"
			]
		}
	]
}
