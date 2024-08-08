import * as gdal from 'gdal-async'

console.log(`GDAL Version: ${gdal.version}, source: ${gdal.bundled ? 'bundled' : 'system library'}`)

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
