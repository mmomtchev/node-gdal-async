import * as gdal from 'gdal-async'
import * as os from 'os'

let noFailNet = function (this: Mocha.Context) {
  let test: Mocha.Suite | Mocha.Test | undefined = this.currentTest
  while (test) {
    if (test.title.match(/w\/Net/)) {
      this.retries(3)
    }
    test = test.parent
  }
}

if (!process.env.MOCHA_TEST_NETWORK || +process.env.MOCHA_TEST_NETWORK === 0) {
  noFailNet = function (this: Mocha.Context) {
    let test: Mocha.Suite | Mocha.Test | undefined = this.currentTest
    while (test) {
      if (test.title.match(/w\/Net/)) {
        console.log('test requires networking, run with MOCHA_TEST_NETWORK=1 npm test to enable:', test.title)
        this.skip()
      }
      test = test.parent
    }
  }
}

const platformSkip = function (this: Mocha.Context) {
  let test: Mocha.Suite | Mocha.Test | undefined = this.currentTest
  while (test) {
    if (test.title.match(/on Linux/) && os.platform() != 'linux') {
      console.log('test requires Linux: ', test.title)
      this.skip()
    }
    test = test.parent
  }
}

const bundledSkip = function (this: Mocha.Context) {
  let test: Mocha.Suite | Mocha.Test | undefined = this.currentTest
  while (test) {
    if (test.title.match(/with bundled GDAL/) && !gdal.bundled) {
      console.log('test requires bundled version of GDAL: ', test.title)
      this.skip()
    }
    test = test.parent
  }
}

const cleanup = () => {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  delete (gdal as any).drivers
  global.gc()
}

exports.mochaHooks = {
  beforeEach: function (done: () => void) {
    platformSkip.call(this)
    noFailNet.call(this)
    bundledSkip.call(this)
    done()
  },
  afterAll: cleanup
}
