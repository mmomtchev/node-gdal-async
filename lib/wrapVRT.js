const { fragment } = require('xmlbuilder2')

/**
 * @typedef {object} VRTBandDescriptor
 * @property {RasterBand[]} sources Source data raster bands
 * @property {string} [pixelFunc] Pixel function to be applied when reading data,
 * must be a GDAL builtin function or a registered user function
 * @property {Record<string, string|number>} [pixelFuncArgs] Additional arguments for the pixel function
 * @property {string} [dataType] Data type to convert the pixels to
 * @property {string} [sourceTransferType] Data type to be used as input of the pixel function
 * when reading from the source
 * @property {string} [description] Optional band description
 */


/**
 * @typedef {object} VRTDescriptor
 * @property {VRTBandDescriptor[]} bands
 */

/**
 * Produces a VRT Dataset from a regular Dataset.
 *
 * Supports applying pixel functions.
 *
 * @example
 * // create a VRT dataset with a single band derived from the first
 * // band of the given dataset by applying the given pixel function
 * const ds = gdal.open(gdal.wrapVRT({
 *  bands: [
 *    {
 *      sources: [ gdal.open('test/data/sample.tif').bands.get(1) ],
 *      pixelFunc: 'expression',
 *      pixelFuncArgs: { dialect: 'exprtk', expression: 'B1 ? 1.5*B3 : B1' },
 *      type: gdal.GDT_Int16,
 *      sourceTransferType: gdal.GDT_Float32
 *    }
 *  ]
 * }));
 *
 * // create a VRT dataset by applying a pixel function over
 * // all bands of the given dataset
 * const ds = gdal.open(gdal.wrapVRT({
 *    bands: gdal.open('test/data/multiband.tif').bands.map((b) => ({
 *      sources: [ b ],
 *      pixelFunc: 'inv',
 *      pixelFuncArgs: { k: 3 }
 *    }))
 * }));
 *
 * // use a pixel function to combine two bands from two files in one band
 * const ds = gdal.open(gdal.wrapVRT({
 *  bands: [
 *    {
 *      sources: [
 *        gdal.open(path.join('test', 'data', 'AROME_T2m_10.tiff')).bands.get(1),
 *        gdal.open(path.join('test', 'data', 'AROME_D2m_10.tiff')).bands.get(1)
 *      ],
 *      // This supposes that the pixel function espy has been
 *      // registered using createPixelFunc
 *      pixelFunc: 'espy'
 *    }
 *  ]
 * }));
 *
 * @param {VRTDescriptor} desc Band descriptors
 * @method wrapVRT
 * @returns {string}
 */

function wrapVRT(desc) {
  if (!desc || !desc.bands || !desc.bands[0] || !desc.bands[0].sources ||
    !desc.bands[0].sources[0] || !desc.bands[0].sources[0].ds) {
    throw new TypeError('Dataset must have at least one RasterBand')
  }
  const ds = desc.bands[0].sources[0].ds
  const root = fragment().ele('VRTDataset', {
    rasterXSize: ds.rasterSize.x,
    rasterYSize: ds.rasterSize.y
  })

  if (ds.srs) {
    const authName = ds.srs.getAuthorityName()
    const authCode = ds.srs.getAuthorityCode()
    if (authName && authCode) {
      root.ele('SRS').txt(`${authName}:${authCode}`)
    } else {
      root.ele('SRS').txt(ds.srs.toWKT())
    }
  }

  if (ds.geoTransform) {
    root.ele('GeoTransform').txt(ds.geoTransform.join(', '))
  }

  const md = ds.getMetadata()
  if (Object.keys(md).length > 0) {
    const xml = root.ele('Metadata')
    Object.keys(md).map((key) => xml.ele('MDI', { key }).txt(md[key]))
  }

  let bandIdx = 1
  for (const band of desc.bands) {
    if (!band.sources || !band.sources[0]) {
      throw new TypeError('Every band must contain at least one source RasterBand')
    }
    const description = band.description || band.sources[0].description

    const xml = root.ele('VRTRasterBand', {
      dataType: band.dataType || band.sources[0].dataType,
      band: bandIdx,
      subClass: band.pixelFunc && 'VRTDerivedRasterBand'
    })
    if (description) xml.ele('Description').txt(description)
    if (band.pixelFunc) xml.ele('PixelFunctionType').txt(band.pixelFunc)

    if (band.pixelFuncArgs) {
      xml.ele('PixelFunctionArguments', band.pixelFuncArgs)
    }

    if (band.sourceTransferType) {
      xml.ele('SourceTransferType').txt(band.sourceTransferType)
    }

    const md = band.sources[0].getMetadata()
    if (Object.keys(md).length > 0) {
      const frag = xml.ele('Metadata')
      Object.keys(md).map((key) => frag.ele('MDI', { key }).txt(md[key]))
    }

    for (const source of band.sources) {
      xml.ele('SimpleSource')
        .ele('SourceFilename', { relativeToVRT: 0 }).txt(source.ds.getFileList()[0]).up()
        .ele('SourceBand').txt(source.id)
    }

    bandIdx++
  }

  return root.end({ prettyPrint: true })
}

module.exports = wrapVRT
