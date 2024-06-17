{
	"includes": [
		"common.gypi"
	],
	"variables": {
		"shared_gdal%": "false",
		"runtime_link%": "shared",
		"enable_logging%": "false",
		"enable_asan%": "false",
		"enable_coverage%": "false",
		"sources_node_gdal": [
				"src/utils/typed_array.cpp",
				"src/utils/string_list.cpp",
				"src/utils/number_list.cpp",
				"src/utils/warp_options.cpp",
				"src/utils/ptr_manager.cpp",
				"src/node_gdal.cpp",
				"src/async.cpp",
				"src/gdal_common.cpp",
				"src/gdal_dataset.cpp",
				"src/gdal_driver.cpp",
				"src/gdal_rasterband.cpp",
				"src/gdal_group.cpp",
				"src/gdal_mdarray.cpp",
				"src/gdal_dimension.cpp",
				"src/gdal_attribute.cpp",
				"src/gdal_majorobject.cpp",
				"src/gdal_feature.cpp",
				"src/gdal_feature_defn.cpp",
				"src/gdal_field_defn.cpp",
				"src/geometry/gdal_geometry.cpp",
				"src/geometry/gdal_point.cpp",
				"src/geometry/gdal_simplecurve.cpp",
				"src/geometry/gdal_compoundcurve.cpp",
				"src/geometry/gdal_linestring.cpp",
				"src/geometry/gdal_circularstring.cpp",
				"src/geometry/gdal_linearring.cpp",
				"src/geometry/gdal_polygon.cpp",
				"src/geometry/gdal_geometrycollection.cpp",
				"src/geometry/gdal_multipoint.cpp",
				"src/geometry/gdal_multilinestring.cpp",
				"src/geometry/gdal_multicurve.cpp",
				"src/geometry/gdal_multipolygon.cpp",
				"src/gdal_layer.cpp",
				"src/gdal_coordinate_transformation.cpp",
				"src/gdal_spatial_reference.cpp",
				"src/gdal_warper.cpp",
				"src/gdal_algorithms.cpp",
				"src/gdal_memfile.cpp",
				"src/gdal_utils.cpp",
				"src/gdal_fs.cpp",
				"src/collections/dataset_bands.cpp",
				"src/collections/dataset_layers.cpp",
				"src/collections/layer_features.cpp",
				"src/collections/layer_fields.cpp",
				"src/collections/group_groups.cpp",
				"src/collections/group_arrays.cpp",
				"src/collections/group_attributes.cpp",
				"src/collections/group_dimensions.cpp",
				"src/collections/array_dimensions.cpp",
				"src/collections/array_attributes.cpp",
				"src/collections/feature_fields.cpp",
				"src/collections/feature_defn_fields.cpp",
				"src/collections/geometry_collection_children.cpp",
				"src/collections/polygon_rings.cpp",
				"src/collections/linestring_points.cpp",
				"src/collections/compound_curves.cpp",
				"src/collections/rasterband_overviews.cpp",
				"src/collections/rasterband_pixels.cpp",
				"src/collections/gdal_drivers.cpp",
				"src/collections/colortable.cpp"
			]
	},
	"conditions": [
		["enable_asan == 'true'", {
			"variables": {
				"debug_extra_ccflags_cc": [ "-fsanitize=address" ],
				"debug_extra_ldflags" : [ "-fsanitize=address" ],
				# using ASan in a Node.js add-on on Windows requires an /MDd build (RuntimeLibrary: 3)
				# which is not a safe build, so this option is to remain out of reach of children
				# (see https://github.com/nodejs/node-gyp/issues/1686)
				"debug_extra_VCCLCompilerTool": [ "/fsanitize=address" ],
				"debug_extra_VCLinkerTool" : [ "/fsanitize=address" ]
			}
		}],
		["enable_coverage == 'true'", {
			"variables": {
				"debug_extra_ccflags_cc": [ "-fprofile-arcs", "-ftest-coverage" ],
				"debug_extra_ldflags" : [ "-lgcov", "--coverage" ],
			},
		}],
		["OS == 'mac'", {
			"variables": {
				"debug_extra_ldflags": [ "-Wl,-bind_at_load" ],
			}
		}],
		["OS == 'linux'", {
			"variables": {
				"debug_extra_ldflags": [ "-Wl,-z,now" ],
			}
		}]
	],
	"targets": [
		{
			"target_name": "gdal",
			"type": "loadable_module",
			"win_delay_load_hook": "false",
			"product_prefix": "",
			"product_extension": "node",
			"sources": [ "<@(sources_node_gdal)" ],
			"include_dirs": [
				"include",
				"<!(node -e \"require('nan')\")"
			],
			"defines": [
				"PLATFORM='<(OS)'",
				"_LARGEFILE_SOURCE",
				"_FILE_OFFSET_BITS=64"
			],
			"xcode_settings": {
				"GCC_ENABLE_CPP_EXCEPTIONS": "YES"
			},
			"conditions": [
				["enable_logging != 'false'", {
					"defines": [
						"ENABLE_LOGGING=1"
					]
				}],
				["shared_gdal == 'false'", {
					"defines": [
						"BUNDLED_GDAL=1"
					],
					"dependencies": [
						"deps/libgdal/libgdal.gyp:libgdal",
						"deps/libproj/libproj.gyp:libproj"
					]
				}, {
					"conditions": [
						['OS == "win"', {
							"include_dirs": [
								"deps/libgdal/arch/win",
								"deps/libgdal/gdal",
								"deps/libgdal/gdal/alg",
								"deps/libgdal/gdal/gcore",
								"deps/libgdal/gdal/port",
								"deps/libgdal/gdal/bridge",
								"deps/libgdal/gdal/frmts",
								"deps/libgdal/gdal/ogr",
								"deps/libgdal/gdal/ogr/ogrsf_frmts",
								"deps/libgdal/gdal/ogr/ogrsf_frmts/mem"
							],
						}, {
							"conditions": [
								["runtime_link == 'static'", {
									"libraries": ["<!@(gdal-config --dep-libs)","<!@(gdal-config --libs)"]
								}, {
									"libraries": ["<!@(gdal-config --libs)"]
								}]
							],
							"cflags_cc": ["<!@(gdal-config --cflags)"],
							"xcode_settings": {
								"OTHER_CPLUSPLUSFLAGS":[
									"<!@(gdal-config --cflags)"
								]
							}
						}]
					]
				}]
			]
		},
		{
			"target_name": "action_after_build",
			"type": "none",
			"dependencies": [ "<(module_name)" ],
			"conditions": [
				['OS != "win"', {
					"actions": [
						{
							"action_name": "yatag",
							"inputs":	[ "<@(sources_node_gdal)", "lib/gdal.js" ],
							"outputs": [ "../lib/index.d.ts" ],
							"action": [ "npm", "run", "yatag" ]
						}
					]
				}],
				['OS == "win"', {
					"actions": [
						{
							"action_name": "yatag",
							"inputs":	[ "<@(sources_node_gdal)", "lib/gdal.js" ],
							"outputs": [ "lib/index.d.ts" ],
							"action": [ 'cmd', '/c"npm run yatag"' ]
						}
					]
				}],
			],
			"copies": [
				{
					"files": [
						"<(PRODUCT_DIR)/gdal.node"
					],
					"destination": "<(module_path)"
				}
			]
		}
	]
}
