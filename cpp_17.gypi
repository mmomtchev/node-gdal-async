{
  # node-gyp sets the standard for us and ensures that its include file comes last
  # this is the only reliable way to override it
  'cflags_cc/': [ ['exclude', '^-std=(?!gnu\\+\\+17)'] ],
  'cflags_cc':[ '-std=gnu++17' ],
  'xcode_settings': {
    'OTHER_CPLUSPLUSFLAGS': [
      '-std=gnu++17'
    ],
    'OTHER_CPLUSPLUSFLAGS/': [ ['exclude', '^-std=(?!gnu\\+\\+17)'] ],
  },
  'msvs_settings': {
    'VCCLCompilerTool': {
      'AdditionalOptions': [
        '/std:c++17'
      ],
      'AdditionalOptions/': [
        ['exclude', '^[/\\-]std:(?!c\\+\\+17)']
      ]
    }
  }
}
