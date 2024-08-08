import * as gdal from 'gdal-async'
import structuredClone from '@ungap/structured-clone'

console.log(`GDAL Version: ${gdal.version}, source: ${gdal.bundled ? 'bundled' : 'system library'}`)
if (!('structuredClone' in globalThis)) {
  console.log('Using a JS implementation of structuredClone')
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  globalThis.structuredClone = structuredClone as any
}

// gc tracing
try {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  (gdal as any).startLogging(`${__dirname}/artifacts/log.txt`)
} catch (_e) {
  /* ignore */
}

// seg fault handler
let SegfaultHandler
try {
  // eslint-disable-next-line @typescript-eslint/no-require-imports
  SegfaultHandler = require('segfault-handler')
  SegfaultHandler.registerHandler()
} catch (_err) {
  /* ignore */
}
