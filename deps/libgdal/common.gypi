{
	"includes": [
		"../../common.gypi"
	],
	"variables": {
		"shared_geos%": "false",
		"endianness": "<!(<(python) -c \"import sys;print(sys.byteorder.upper())\")",
	},
	"target_defaults": {
		"include_dirs": [
			"./gdal/alg",
			"./gdal/gcore",
			"./gdal/apps",
			"./gdal/port",
			"./gdal/frmts",
			"./gdal/frmts/gtiff",
			"./gdal/frmts/vrt",
			"./gdal/frmts/mem",
			"./gdal/ogr",
			"./gdal/ogr/ogrsf_frmts",
			"./gdal/ogr/ogrsf_frmts/mem",
			"./gdal/ogr/ogrsf_frmts/generic",
			"./gdal/ogr/ogrsf_frmts/shape",
			"./gdal/ogr/ogrsf_frmts/avc",
			"./gdal/ogr/ogrsf_frmts/vrt",
			"./gdal/ogr/ogrsf_frmts/geojson",
			"./gdal/ogr/ogrsf_frmts/geojson/libjson"
		],
		"defines": [
			"_LARGEFILE_SOURCE",
			"_FILE_OFFSET_BITS=64",
			"INTERNAL_LIBGEOTIFF=1",
			"GDAL_COMPILATION=1",
			"PAM_ENABLED=1",
			"OGR_ENABLED=1",
			"HAVE_DECL_NAN=1",
			"HAVE_DECL_ISNAN=1",
			"HAVE_DECL_INFINITY=1",
			"HAVE_DECL_ISINF=1",
			"HAVE_TIFF=1",
			"HAVE_EXPAT=1",
			"HAVE_LIBPROJ=1",
			"HAVE_GEOS=1",
			"HAVE_SQLITE=1",
			"HAVE_SQLITE3EXT_H=1",
			"HAVE_MITAB=1",
			"HAVE_CURL=1",
			"HAVE_HDF5=1",
			"USE_PNG=1",
			"USE_AEC=1",
			"NETCDF_HAS_NC4=1",
			"PROJ_STATIC=1",
			"CPU_<(endianness)_ENDIAN=1",
			"BIGTIFF_SUPPORT=1",
			"ZIP_SUPPORT=1",
			"HAVE_LIBZ=1",
			"JPEG_SUPPORT=1",
			"RENAME_INTERNAL_SHAPELIB_SYMBOLS=1",
			"GDAL_USE_SHAPELIB_INTERNAL=1",
			"flatbuffers=gdal_flatbuffers"
		],
		"dependencies": [
			"<(deps_dir)/libexpat/libexpat.gyp:libexpat",
			"<(deps_dir)/libproj/libproj.gyp:libproj",
			"<(deps_dir)/libsqlite/libsqlite.gyp:libsqlite",
			"<(deps_dir)/libcurl/libcurl.gyp:libcurl",
			"<(deps_dir)/libhdf5/libhdf5.gyp:libhdf5",
			"<(deps_dir)/libnetcdf/libnetcdf.gyp:libnetcdf",
			"<(deps_dir)/libopenjpeg/libopenjpeg.gyp:libopenjpeg",
			"<(deps_dir)/libaec/libaec.gyp:libaec"
		],
		"conditions": [
			["OS == 'win'", {
				"include_dirs": ["./arch/win"],
				"VCCLCompilerTool": {
					"DebugInformationFormat": "0"
				},
				"VCLinkerTool": {
					"GenerateDebugInformation": "false",
				},
			}],
			["OS == 'linux'", {
				"defines": [
					"ENABLE_UFFD=1",
					"HAVE_5ARGS_MREMAP=1",
					"HAVE_SC_PHYS_PAGES=1"
				]
			}],
			["OS == 'freebsd'", {
				"include_dirs": ["./arch/bsd"]
			}],
			["OS != 'freebsd' and OS != 'win'", {
				"include_dirs": ["./arch/unix"],
				"defines": [ "HAVE_GETRLIMIT=1" ]
			}],
			["shared_geos == 'false'", {
				"dependencies": [
					"<(deps_dir)/libgeos/libgeos.gyp:libgeos"
				]
			}]
		],
	}
}
