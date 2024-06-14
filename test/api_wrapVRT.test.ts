import * as gdal from 'gdal-async'
import * as chai from 'chai'
import * as path from 'path'
import * as semver from 'semver'
const assert: Chai.Assert = chai.assert

describe('gdal.wrapVRT()', () => {
  const sample = path.resolve(__dirname, 'data', 'sample.tif')
  const multiband = path.resolve(__dirname, 'data', 'multiband.tif')
  const arome_t2m = path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')
  const arome_d2m = path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')

  it('should produce identical Dataset by default', function () {
    if (!semver.gte(gdal.version, '3.4.0')) {
      this.skip()
    }
    const expected = `<VRTDataset rasterXSize="984" rasterYSize="804">
  <SRS>PROJCS["unnamed",GEOGCS["GRS 1980(IUGG, 1980)",DATUM["unknown",SPHEROID["GRS80",6378137,298.257222101],TOWGS84[0,0,0,0,0,0,0]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]]],PROJECTION["Albers_Conic_Equal_Area"],PARAMETER["latitude_of_center",23],PARAMETER["longitude_of_center",-96],PARAMETER["standard_parallel_1",29.5],PARAMETER["standard_parallel_2",45.5],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH]]</SRS>
  <GeoTransform>-1134675.2952829634, 7.502071930146189, 0, 2485710.4658232867, 0, -7.502071930145942</GeoTransform>
  <Metadata>
    <MDI key="AREA_OR_POINT">Area</MDI>
  </Metadata>
  <VRTRasterBand dataType="Byte" band="1">
    <SimpleSource>
      <SourceFilename relativeToVRT="0">${sample}</SourceFilename>
      <SourceBand>1</SourceBand>
    </SimpleSource>
  </VRTRasterBand>
</VRTDataset>`
    assert.equal(gdal.wrapVRT({ bands: [
      { sources: [ gdal.open(sample).bands.get(1) ] } ]
    }), expected)
  })

  it('should apply a pixel function when specified', function () {
    if (!semver.gte(gdal.version, '3.4.0')) {
      this.skip()
    }
    const expected = `<VRTDataset rasterXSize="984" rasterYSize="804">
  <SRS>PROJCS["unnamed",GEOGCS["GRS 1980(IUGG, 1980)",DATUM["unknown",SPHEROID["GRS80",6378137,298.257222101],TOWGS84[0,0,0,0,0,0,0]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]]],PROJECTION["Albers_Conic_Equal_Area"],PARAMETER["latitude_of_center",23],PARAMETER["longitude_of_center",-96],PARAMETER["standard_parallel_1",29.5],PARAMETER["standard_parallel_2",45.5],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH]]</SRS>
  <GeoTransform>-1134675.2952829634, 7.502071930146189, 0, 2485710.4658232867, 0, -7.502071930145942</GeoTransform>
  <Metadata>
    <MDI key="AREA_OR_POINT">Area</MDI>
  </Metadata>
  <VRTRasterBand dataType="Int16" band="1" subClass="VRTDerivedRasterBand">
    <PixelFunctionType>inv</PixelFunctionType>
    <PixelFunctionArguments k="3"/>
    <SourceTransferType>Float32</SourceTransferType>
    <SimpleSource>
      <SourceFilename relativeToVRT="0">${sample}</SourceFilename>
      <SourceBand>1</SourceBand>
    </SimpleSource>
  </VRTRasterBand>
</VRTDataset>`

    assert.equal(gdal.wrapVRT({
      bands: [
        {
          sources: [ gdal.open(sample).bands.get(1) ],
          pixelFunc: 'inv',
          pixelFuncArgs: { k: 3 },
          dataType: gdal.GDT_Int16,
          sourceTransferType: gdal.GDT_Float32
        }
      ]
    }), expected)
  })

  it('should support multiband files', function () {
    if (!semver.gte(gdal.version, '3.4.0')) {
      this.skip()
    }
    const ds = gdal.open(multiband, 'r')
    const metadata = ds.getMetadata()
    const expected = `<VRTDataset rasterXSize="512" rasterYSize="512">
  <SRS>EPSG:3857</SRS>
  <GeoTransform>-11584184.5107, 30.00058143924877, 0, 5635549.221409999, 0, -30.00058143924877</GeoTransform>
  <Metadata>
${Object.keys(metadata).map((k) => `    <MDI key="${k}">${metadata[k]}</MDI>`).join('\n')}
  </Metadata>
  <VRTRasterBand dataType="Byte" band="1">
    <SimpleSource>
      <SourceFilename relativeToVRT="0">${multiband}</SourceFilename>
      <SourceBand>1</SourceBand>
    </SimpleSource>
  </VRTRasterBand>
  <VRTRasterBand dataType="Byte" band="2">
    <SimpleSource>
      <SourceFilename relativeToVRT="0">${multiband}</SourceFilename>
      <SourceBand>2</SourceBand>
    </SimpleSource>
  </VRTRasterBand>
  <VRTRasterBand dataType="Byte" band="3">
    <SimpleSource>
      <SourceFilename relativeToVRT="0">${multiband}</SourceFilename>
      <SourceBand>3</SourceBand>
    </SimpleSource>
  </VRTRasterBand>
</VRTDataset>`

    const bands = ds.bands.map((b) => ({ sources: [ b ] }))

    const actual = gdal.wrapVRT({bands})

    assert.strictEqual(actual, expected)
  })

  it('should support combining files', function () {
    if (!semver.gte(gdal.version, '3.4.0')) {
      this.skip()
    }
    const expected = `<VRTDataset rasterXSize="801" rasterYSize="601">
  <SRS>GEOGCS["Coordinate System imported from GRIB file",DATUM["unknown",SPHEROID["Sphere",6367470,0]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AXIS["Latitude",NORTH],AXIS["Longitude",EAST]]</SRS>
  <GeoTransform>-8.0125, 0.025, 0, 53.0125, 0, -0.025</GeoTransform>
  <Metadata>
    <MDI key="AREA_OR_POINT">Point</MDI>
  </Metadata>
  <VRTRasterBand dataType="Float64" band="1" subClass="VRTDerivedRasterBand">
    <Description>10[m] HTGL="Specified height level above ground"</Description>
    <PixelFunctionType>espy</PixelFunctionType>
    <Metadata>
      <MDI key="GRIB_COMMENT">Temperature [C]</MDI>
      <MDI key="GRIB_ELEMENT">TMP</MDI>
      <MDI key="GRIB_FORECAST_SECONDS">36000 sec</MDI>
      <MDI key="GRIB_REF_TIME">1600473600 sec UTC</MDI>
      <MDI key="GRIB_SHORT_NAME">10-HTGL</MDI>
      <MDI key="GRIB_UNIT">[C]</MDI>
      <MDI key="GRIB_VALID_TIME">1600509600 sec UTC</MDI>
    </Metadata>
    <SimpleSource>
      <SourceFilename relativeToVRT="0">${arome_t2m}</SourceFilename>
      <SourceBand>1</SourceBand>
    </SimpleSource>
    <SimpleSource>
      <SourceFilename relativeToVRT="0">${arome_d2m}</SourceFilename>
      <SourceBand>1</SourceBand>
    </SimpleSource>
  </VRTRasterBand>
</VRTDataset>`

    assert.equal(gdal.wrapVRT({
      bands: [
        {
          sources: [
            gdal.open(arome_t2m).bands.get(1),
            gdal.open(arome_d2m).bands.get(1)
          ],
          pixelFunc: 'espy'
        }
      ] }),
    expected)
  })

  it('should throw with invalid arguments', function () {
    if (!semver.gte(gdal.version, '3.4.0')) {
      this.skip()
    }
    assert.throws(() => {
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      (gdal.wrapVRT as any)()
    }, /Dataset must have at least one RasterBand/)

    assert.throws(() => {
      gdal.wrapVRT({ bands: [] })
    }, /Dataset must have at least one RasterBand/)

    assert.throws(() => {
      gdal.wrapVRT({
        bands: [ { sources: [] } ] })
    }, /Dataset must have at least one RasterBand/)

    assert.throws(() => {
      gdal.wrapVRT({
        bands: [ { sources: [ {} as gdal.RasterBand ] } ] })
    }, /Dataset must have at least one RasterBand/)
  })
})
