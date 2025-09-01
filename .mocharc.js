module.exports = {
  spec: 'test/*.test.ts',
  require: [
    'ts-node/register/transpile-only',
    'tsconfig-paths/register',
    'test/_common.ts',
    'test/_hooks.ts'
  ],
  reporter: 'tap',
  timeout: 20000,
  // Keep Node options minimal and stable; avoid experimental ESM loader
  'node-option': (() => {
    const [ majorStr, minorStr ] = process.versions.node.split('.')
    const major = parseInt(majorStr, 10)
    const minor = parseInt(minorStr, 10)
    const opts = [ 'expose-gc' ]
    // Keep previous workaround for Node 20.19+ to silence require() of ESM warnings
    if (major === 20 && minor >= 19) opts.unshift('no-experimental-require-module')
    return opts
  })()
}
