{
	"includes": [
		"../../common.gypi"
	],
	'target_defaults': {
		'default_configuration': 'Release',
		'configurations': {
			'Debug': {
				'defines': [ 'DEBUG', '_DEBUG' ],
			},
			'Release': {
				'defines': [ 'NDEBUG' ],
			}
		}
	},
	'targets': [
		{
			'target_name': 'muparser',
			'type': 'static_library',
			'include_dirs': [ './muparser/include' ],
			'defines': [ 'MUPARSER_STATIC' ],
			'sources': [
        '<!@(<(python) ../glob-files.py "muparser/src/*.cpp")'
			],
			'direct_dependent_settings': {
				'include_dirs': [ './muparser/include' ],
				'defines': [ 'MUPARSER_STATIC' ]
			}
		}
	]
}
