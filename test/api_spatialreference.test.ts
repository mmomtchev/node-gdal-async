import * as fs from 'fs'
import * as gdal from 'gdal-async'
import * as chaiAsPromised from 'chai-as-promised'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
chai.use(chaiAsPromised)
import * as semver from 'semver'

// http://epsg.io/
// http://spatialreference.org/ref/

describe('gdal.SpatialReference', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  it('should be exposed', () => {
    assert.ok(gdal.SpatialReference)
  })
  it('should be instantiable', () => {
    assert.instanceOf(new gdal.SpatialReference(), gdal.SpatialReference)
  })
  it('should be instantiable w/WKT', () => {
    assert.instanceOf(new gdal.SpatialReference(
      'GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]]'),
    gdal.SpatialReference)
  })
  it('constructor can throw', () => {
    assert.throws(() => {
      new gdal.SpatialReference('c\'est en faisant n\'importe quoi')
    })
  })
  describe('fromWKT()', () => {
    it('should return SpatialReference', () => {
      const wkt =
        'PROJCS["NAD_1983_UTM_Zone_10N",' +
        'GEOGCS["GCS_North_American_1983",' +
        'DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137,298.257222101]],' +
        'PRIMEM["Greenwich",0],UNIT["Degree",0.0174532925199433]],' +
        'PROJECTION["Transverse_Mercator"],PARAMETER["False_Easting",500000.0],' +
        'PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",-123.0],' +
        'PARAMETER["Scale_Factor",0.9996],PARAMETER["Latitude_of_Origin",0.0],' +
        'UNIT["Meter",1.0]]'

      const ref = gdal.SpatialReference.fromWKT(wkt)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid WKT', () => {
      if (semver.gte(gdal.version, '3.0.0')) {
        const wkt = 'Corneilles["ébouriffées"]'
        assert.throws(() => {
          gdal.SpatialReference.fromWKT(wkt)
        })
      }
    })
  })
  describe('fromProj4()', () => {
    it('should return SpatialReference', () => {
      const proj =
        '+proj=stere +lat_ts=-37 +lat_0=-90 +lon_0=145 +k_0=1.0 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs'
      const ref = gdal.SpatialReference.fromProj4(proj)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid Proj4', () => {
      const proj =
        '+proj=stereo +lat_ts=-37 +lat_0=-90 +lon_0=145 +k_0=1.0 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs'
      assert.throws(() => {
        gdal.SpatialReference.fromWKT(proj)
      })
    })
  })
  describe('fromEPSG()', () => {
    it('should return SpatialReference', () => {
      const epsg = 4326
      const ref = gdal.SpatialReference.fromEPSG(epsg)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid EPSG', () => {
      assert.throws(() => {
        gdal.SpatialReference.fromEPSG(99191)
      })
    })
  })
  describe('fromEPSGA()', () => {
    it('should return SpatialReference', () => {
      const epsga = 26910
      const ref = gdal.SpatialReference.fromEPSGA(epsga)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid EPSGA', () => {
      assert.throws(() => {
        gdal.SpatialReference.fromEPSGA(99191)
      })
    })
  })
  describe('fromESRI', () => {
    it('should return SpatialReference', () => {
      const esri = [
        'GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137,298.257222101]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]'
      ]
      const ref = gdal.SpatialReference.fromESRI(esri)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid EPSGA', () => {
      const esri = [ 'Corneilles["ébouriffées"]' ]
      assert.throws(() => {
        gdal.SpatialReference.fromESRI(esri)
      })
    })
  })
  describe('fromXML', () => {
    it('should return SpatialReference', () => {
      const gml = fs.readFileSync(`${__dirname}/data/srs/sample.gml`, 'utf8')
      const ref = gdal.SpatialReference.fromXML(gml)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid XML', () => {
      assert.throws(() => {
        gdal.SpatialReference.fromXML('<gml:GeographicCRS gml:id="ogrcrs1"></gml:GeographicCRS>')
      })
    })
  })
  describe('fromWMSAUTO', () => {
    it('should return SpatialReference', () => {
      const wms = 'AUTO:42001,99,8888'
      const ref = gdal.SpatialReference.fromWMSAUTO(wms)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid WMS', () => {
      const wms = 'AUTO:00001,XX,invalid'
      assert.throws(() => {
        gdal.SpatialReference.fromWMSAUTO(wms)
      })
    })
  })
  describe('fromURN', () => {
    it('should return SpatialReference', () => {
      const wms = 'urn:ogc:def:crs:EPSG::26912'
      const ref = gdal.SpatialReference.fromURN(wms)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid URN', () => {
      const wms = 'urn:ogc:def:crs:EPSG::99912'
      assert.throws(() => {
        gdal.SpatialReference.fromURN(wms)
      })
    })
  })
  describe('fromUserInputAsync', () => {
    it('should return SpatialReference', () => {
      const wms = 'urn:ogc:def:crs:EPSG::26912'
      const refq = gdal.SpatialReference.fromUserInputAsync(wms)
      return assert.eventually.instanceOf(refq, gdal.SpatialReference)
    })
    it('should reject on invalid URN', () => {
      const wms = 'urn:ogc:def:crs:EPSG::99912'
      return assert.isRejected(gdal.SpatialReference.fromUserInputAsync(wms))
    })
  })
  describe('fromURL w/Net', () => {
    it('should return SpatialReference', () => {
      const ref = gdal.SpatialReference.fromURL('http://spatialreference.org/ref/epsg/4326/')
      assert.instanceOf(ref, gdal.SpatialReference)
      assert.isTrue(ref.isSame(gdal.SpatialReference.fromEPSG(4326)))
    })
    it('should throw on invalid URL', () => {
      assert.throws(() => {
        gdal.SpatialReference.fromURL('http://nosuchdomain/ref/epsg/4326/')
      })
    })
  })
  describe('fromURLAsync w/Net', () => {
    it('should return SpatialReference', () => {
      const refq = gdal.SpatialReference.fromURLAsync('http://spatialreference.org/ref/epsg/4326/')
      return refq.then((ref) => {
        assert.instanceOf(ref, gdal.SpatialReference)
        assert.isTrue(ref.isSame(gdal.SpatialReference.fromEPSG(4326)))
      })
    })
    it('should reject on invalid URL', () =>
      assert.isRejected(gdal.SpatialReference.fromURLAsync('http://nosuchdomain/ref/epsg/4326/'))
    )
  })
  describe('fromCRSURL w/Net', () => {
    it('should return SpatialReference', () => {
      const wms = 'http://www.opengis.net/def/crs/EPSG/0/3857'
      const ref = gdal.SpatialReference.fromCRSURL(wms)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid crs URL', () => {
      const wms = 'http://www.opengis.cz/def/crs/EPSG/0/3857'
      assert.throws(() => {
        gdal.SpatialReference.fromCRSURL(wms)
      })
    })
  })
  describe('fromCRSURLAsync w/Net', () => {
    it('should return SpatialReference', () => {
      const wms = 'http://www.opengis.net/def/crs/EPSG/0/3857'
      const refq = gdal.SpatialReference.fromCRSURLAsync(wms)
      return assert.eventually.instanceOf(refq, gdal.SpatialReference)
    })
    it('should reject on invalid crs URL', () => {
      const wms = 'http://www.opengis.cz/def/crs/EPSG/0/3857'
      return assert.isRejected(gdal.SpatialReference.fromCRSURLAsync(wms))
    })
  })
  describe('fromMICoordSys', () => {
    it('should return SpatialReference', () => {
      const micoordsys = 'Earth Projection 10, 157, "m", 0'
      const ref = gdal.SpatialReference.fromMICoordSys(micoordsys)
      assert.instanceOf(ref, gdal.SpatialReference)
    })
    it('should throw on invalid crs URL', () => {
      const micoordsys = 'Mars Projection 10, 157, "m", 0'
      assert.throws(() => {
        gdal.SpatialReference.fromMICoordSys(micoordsys)
      })
    })
  })
  describe('clone', () => {
    it('should clone a SpatialReference', () => {
      const srs1 = gdal.SpatialReference.fromEPSG(4326)
      const srs2 = srs1.clone()
      assert.isTrue(srs1.isSame(srs2))
      assert.notStrictEqual(srs1, srs2)
    })
  })
  describe('cloneGeogCS', () => {
    it('should clone a SpatialReference', () => {
      const wkt =
        'PROJCS["NAD_1983_UTM_Zone_10N",' +
        'GEOGCS["GCS_North_American_1983",' +
        'DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137,298.257222101]],' +
        'PRIMEM["Greenwich",0],UNIT["Degree",0.0174532925199433]],' +
        'PROJECTION["Transverse_Mercator"],PARAMETER["False_Easting",500000.0],' +
        'PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",-123.0],' +
        'PARAMETER["Scale_Factor",0.9996],PARAMETER["Latitude_of_Origin",0.0],' +
        'UNIT["Meter",1.0]]'
      const srs1 = gdal.SpatialReference.fromWKT(wkt)
      const srs2 = srs1.cloneGeogCS()
      assert.isFalse(srs1.isSame(srs2))
      assert.isTrue(srs1.isSameGeogCS(srs2))
    })
  })
  describe('setWellKnownGeogCS', () => {
    it('should accept a string parameter', () => {
      const srs = new gdal.SpatialReference()
      srs.setWellKnownGeogCS('EPSG:4326')
      assert.isTrue(srs.isSame(gdal.SpatialReference.fromEPSG(4326)))
    })
    it('should throw on invalid input', () => {
      const srs = new gdal.SpatialReference()
      assert.throws(() => {
        srs.setWellKnownGeogCS('invalid')
      })
    })
  })
  describe('validate', () => {
    it('should validate a valid SpatialReference', () => {
      const epsg = 4326
      const ref = gdal.SpatialReference.fromEPSG(epsg)
      assert.isNull(ref.validate())
    })
    if (semver.gte(gdal.version, '3.0.0')) {
      it('should not validate an invalid SpatialReference', () => {
        const wkt =
          'PROJCS["NAD_1983_UTM_Zone_10N",' +
          'GEOGCS["GCS_North_American_1983",' +
          'DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137,298.257222101]],' +
          'PRIMEM["Greenwich",0],UNIT["Degree",0.0174532925199433]],' +
          'PROJECTION["Transverse_Mercator"],PARAMETER["False_Easting",500000.0],' +
          'PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",-123.0],' +
          'PARAMETER["Scale_Factor",0.9996],PARAMETER["Latitude_of_Origin",0.0],' +
          'PARAMETER["Roshavi_Gargi", 2]' +
          'UNIT["Meter",1.0]]'
        const ref = gdal.SpatialReference.fromWKT(wkt)
        assert.isNotNull(ref.validate())
      })
    }
  })
  describe('exportToPrettyWKT', () => {
    it('should pretty-print WKT', () => {
      const ref = gdal.SpatialReference.fromEPSG(3857)
      assert.include(ref.toPrettyWKT(), 'WGS 84 / Pseudo-Mercator')
    })
  })
  describe('exportToXML', () => {
    it('should export to XML', () => {
      const epsg = 3857
      const ref = gdal.SpatialReference.fromEPSG(epsg)
      assert.include(ref.toXML(), 'WGS 84 / Pseudo-Mercator')
    })
  })
  describe('getAngularUnits', () => {
    it('should retrieve the angular units of a SpatialReference', () => {
      const ref = gdal.SpatialReference.fromEPSG(3857)
      const units = ref.getAngularUnits()
      assert.closeTo(units.value, 0.0174, 0.001)
      assert.equal(units.units, 'degree')
    })
  })
  describe('getLinearUnits', () => {
    it('should retrieve the linear units of a SpatialReference', () => {
      const ref = gdal.SpatialReference.fromEPSG(3857)
      const units = ref.getLinearUnits()
      assert.closeTo(units.value, 1, 0.001)
      assert.equal(units.units, 'metre')
    })
  })
  describe('getAuthorityCode', () => {
    it('should support string argument', () => {
      const srs = gdal.SpatialReference.fromUserInput('EPSG:27700')
      assert.strictEqual(srs.getAuthorityCode('PROJCS'), '27700')
    })
    it('should support null argument', () => {
      // https://github.com/naturalatlas/node-gdal/issues/218
      const wkt =
        'GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],AXIS["Latitude",NORTH],AXIS["Longitude",EAST],UNIT["Degree",0.0174532925199433]]'
      const srs = gdal.SpatialReference.fromWKT(wkt)
      srs.autoIdentifyEPSG()
      assert.strictEqual(srs.getAuthorityCode(null), '4269')
    })
    it('should support no arguments', () => {
      const wkt =
        'GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],AXIS["Latitude",NORTH],AXIS["Longitude",EAST],UNIT["Degree",0.0174532925199433]]'
      const srs = gdal.SpatialReference.fromWKT(wkt)
      srs.autoIdentifyEPSG()
      assert.strictEqual(srs.getAuthorityCode(), '4269')
    })
  })
  describe('getAuthorityName', () => {
    it('should support string argument', () => {
      const srs = gdal.SpatialReference.fromUserInput('EPSG:27700')
      assert.strictEqual(srs.getAuthorityName('PROJCS'), 'EPSG')
    })
    it('should support null argument', () => {
      const wkt =
        'GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],AXIS["Latitude",NORTH],AXIS["Longitude",EAST],UNIT["Degree",0.0174532925199433]]'
      const srs = gdal.SpatialReference.fromWKT(wkt)
      srs.autoIdentifyEPSG()
      assert.strictEqual(srs.getAuthorityName(null), 'EPSG')
    })
    it('should support no arguments', () => {
      const wkt =
        'GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],AXIS["Latitude",NORTH],AXIS["Longitude",EAST],UNIT["Degree",0.0174532925199433]]'
      const srs = gdal.SpatialReference.fromWKT(wkt)
      srs.autoIdentifyEPSG()
      assert.strictEqual(srs.getAuthorityName(), 'EPSG')
    })
  })
  describe('getAttrValue', () => {
    it('should support default index argument', () => {
      const srs = gdal.SpatialReference.fromEPSG(3857)
      assert.strictEqual(srs.getAttrValue('DATUM'), 'WGS_1984')
    })
    it('should support custom index argument', () => {
      const srs = gdal.SpatialReference.fromEPSG(3857)
      assert.strictEqual(srs.getAttrValue('DATUM', 1), 'SPHEROID')
    })
    it('should support invalid index argument', () => {
      const srs = gdal.SpatialReference.fromEPSG(3857)
      assert.isNull(srs.getAttrValue('DATUM', 20))
    })
  })
  describe('toProj4', () => {
    it('should return string', () => {
      const srs = gdal.SpatialReference.fromUserInput('NAD83')
      const expected = [
        '+proj=longlat +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +no_defs', // old gdal
        '+proj=longlat +datum=NAD83 +no_defs' // new gdal
      ]
      assert.include(expected, srs.toProj4())
    })
  })
  describe('isGeographic', () => {
    it('should return true if geographic coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(4326).isGeographic(), true)
    })
    it('should return false if not geographic coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(2154).isGeographic(), false)
    })
  })
  describe('isGeocentric', () => {
    it('should return true if geocentric coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(4328).isGeocentric(), true)
    })
    it('should return false if not geocentric coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(2154).isGeocentric(), false)
    })
  })
  describe('isProjected', () => {
    it('should return true if projected coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(3857).isProjected(), true)
    })
    it('should return false if not projected coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(4326).isProjected(), false)
    })
  })
  describe('isLocal', () => {
    it('should return true if local coordinate system', () => {
      const local = 'LOCAL_CS["PAM",UNIT["metre",1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH]]'
      assert.equal(gdal.SpatialReference.fromWKT(local).isLocal(), true)
    })
    it('should return false if not local coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(2154).isLocal(), false)
    })
  })
  const compoundVertical = 'COMPD_CS["NAD27 / UTM zone 11N + EGM2008 height",PROJCS["NAD27 / UTM zone 11N",GEOGCS["NAD27",DATUM["North_American_Datum_1927",SPHEROID["Clarke 1866",6378206.4,294.978698213898,AUTHORITY["EPSG","7008"]],AUTHORITY["EPSG","6267"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4267"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",-117],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH],AUTHORITY["EPSG","26711"]],VERT_CS["EGM2008 height",VERT_DATUM["EGM2008 geoid",2005,AUTHORITY["EPSG","1027"]],UNIT["foot",0.3048,AUTHORITY["EPSG","9002"]],AXIS["Up",UP]]]'
  describe('isVertical', () => {
    it('should return true if vertical coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromWKT(compoundVertical).isVertical(), true)
      // This is a typo that will probably stay until 4.x
      /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
      assert.equal((gdal.SpatialReference.fromWKT(compoundVertical) as any).isVectical(), true)
    })
    it('should return false if not vertical coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(2154).isVertical(), false)
    })
  })
  describe('isCompound', () => {
    it('should return true if compound coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromWKT(compoundVertical).isCompound(), true)
    })
    it('should return false if not compound coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(2154).isCompound(), false)
    })
  })
  describe('isSame w/Net', () => {
    it('should return true if compound coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(3857)
        .isSame(gdal.SpatialReference.fromCRSURL('http://www.opengis.net/def/crs/EPSG/0/3857')), true)
    })
    it('should return false if not compound coordinate system', () => {
      assert.equal(gdal.SpatialReference.fromEPSG(4326)
        .isSame(gdal.SpatialReference.fromCRSURL('http://www.opengis.net/def/crs/EPSG/0/3857')), false)
    })
  })
})
