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
  // Ensure Mocha can load TypeScript test files in both CJS and ESM contexts
  // Mocha v11+ may use dynamic import for loading test files; using the ts-node ESM loader
  // guarantees TS files with import syntax are handled correctly.
  'node-option': (() => {
    const [ majorStr, minorStr ] = process.versions.node.split('.')
    const major = parseInt(majorStr, 10)
    const minor = parseInt(minorStr, 10)
    const opts = [ 'expose-gc' ]
    // Use ts-node ESM loader when supported (Node >= 16)
    if (major >= 16) opts.unshift('loader=ts-node/esm')
    // Keep previous workaround for Node 20.19+ to silence require() of ESM warnings
    if (major === 20 && minor >= 19) opts.unshift('no-experimental-require-module')
    return opts
  })()
}
