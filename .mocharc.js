module.exports = {
  spec: 'test/*.test.ts',
  require: [
    'ts-node/register',
    'tsconfig-paths/register',
    'test/_common.ts',
    'test/_hooks.ts'
  ],
  reporter: 'tap',
  timeout: 20000,
  'node-option':
      process.versions.node.split('.')[0] == 20 && process.versions.node.split('.')[1] >= 19 ? [
        'no-experimental-require-module', 'expose-gc'
      ] : [ 'expose-gc' ]
}
