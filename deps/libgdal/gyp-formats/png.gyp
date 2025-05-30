{
	"includes": [
		"../common.gypi"
	],
	"targets": [
		{
			"target_name": "libgdal_png_frmt",
			"type": "static_library",
			"defines": [
				"PNG_ARM_NEON_IMPLEMENTATION=0",
				"PNG_ARM_NEON_OPT=0"
			],
			"sources": [
				"../gdal/frmts/png/pngdataset.cpp",
				"../gdal/frmts/png/libpng/pngrio.c",
				"../gdal/frmts/png/libpng/pngerror.c",
				"../gdal/frmts/png/libpng/pngwio.c",
				"../gdal/frmts/png/libpng/pngtrans.c",
				"../gdal/frmts/png/libpng/pngwrite.c",
				"../gdal/frmts/png/libpng/pngwutil.c",
				"../gdal/frmts/png/libpng/pngwtran.c",
				"../gdal/frmts/png/libpng/pngpread.c",
				"../gdal/frmts/png/libpng/pngread.c",
				"../gdal/frmts/png/libpng/pngmem.c",
				"../gdal/frmts/png/libpng/pngget.c",
				"../gdal/frmts/png/libpng/png.c",
				"../gdal/frmts/png/libpng/pngrtran.c",
				"../gdal/frmts/png/libpng/pngrutil.c",
				"../gdal/frmts/png/libpng/pngset.c",
				"../gdal/frmts/png/pngdrivercore.cpp"
			],
			"include_dirs": [
				"../gdal/frmts/png",
				"../gdal/frmts/png/libpng"
			]
		}
	]
}
