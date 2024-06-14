import * as gdal from 'gdal-async'
import * as path from 'path'
import * as assert from 'assert'
import * as semver from 'semver'

describe('Open', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  it('should throw when invalid file', () => {
    const filename = path.join(__dirname, 'data/invalid')
    assert.throws(() => {
      gdal.open(filename)
    }, semver.gte(gdal.version, '3.0.0') ? /not recognized.*supported file format/ : /Error/)
  })
  it('should throw when non-existing file', () => {
    const filename = path.join(__dirname, 'data/inexisting')
    assert.throws(() => {
      gdal.open(filename)
    }, semver.gte(gdal.version, '3.0.0') ? /No such file or directory/ : /Error/)
  })
})
