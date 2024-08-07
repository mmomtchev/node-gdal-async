import { assert } from 'chai'
import * as semver from 'semver'
import * as path from 'path'
import * as gdal from 'gdal-async'

// create = function -> class cannot be directly instantiated
// create = array -> these are the arguments for the new
const create = {
  CircularString: [],
  ColorTable: [ gdal.GPI_HLS ],
  CompoundCurve: [],
  CompoundCurveCurves: () => new gdal.CompoundCurve().curves,
  CoordinateTransformation: [ gdal.SpatialReference.fromEPSG(4326), gdal.SpatialReference.fromEPSG(3857) ],
  Dataset: () => gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte),
  DatasetBands: () => gdal.open(path.resolve(__dirname, 'data', 'sample.tif')).bands,
  DatasetLayers: () => gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')).layers,
  Driver: () => gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte).driver,
  Feature: [ new gdal.FeatureDefn() ],
  FeatureDefn: [],
  FeatureDefnFields: () => new gdal.FeatureDefn().fields,
  FeatureFields: () => gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')).layers.get(0).features.get(0).fields,
  FieldDefn: [ 'id', gdal.OFTInteger ],
  GDALDrivers: () => gdal.drivers,
  Geometry: () => new gdal.LineString(),
  GeometryCollection: [],
  GeometryCollectionChildren: () => new gdal.GeometryCollection().children,
  Layer: () => gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')).layers.get(0),
  LayerFields: () => gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')).layers.get(0).fields,
  LayerFeatures: () => gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')).layers.get(0).features,
  LinearRing: [],
  LineString: [],
  LineStringPoints: () => new gdal.LineString().points,
  MultiCurve: [],
  MultiLineString: [],
  MultiPoint: [],
  MultiPolygon: [],
  Point: [ 0, 0 ],
  Polygon: [],
  PolygonRings: () => new gdal.Polygon().rings,
  RasterBand: () => gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte).bands.get(1),
  RasterBandOverviews: () => gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte).bands.get(1).overviews,
  RasterBandPixels: () => gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte).bands.get(1).pixels,
  SimpleCurve: () => new gdal.LineString(),
  SpatialReference: []
} as Record<string, unknown>

const create31 = {
  Attribute: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.attributes.get(1),
  ArrayAttributes: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.arrays.get(1).attributes,
  ArrayDimensions: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.arrays.get(1).dimensions,
  Dimension: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.dimensions.get(1),
  Group: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root,
  GroupAttributes: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.attributes,
  GroupArrays: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.arrays,
  GroupDimensions: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.dimensions,
  GroupGroups: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.groups,
  MDArray: () => gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr').root.arrays.get(1)
}

describe('Class semantics', () => {
  afterEach(global.gc!)

  const klasses = create
  if (semver.gte(gdal.version, '3.1.0')) {
    Object.assign(klasses, create31)
  }

  for (const name in klasses) {
    it(`gdal.${name}`, () => {
      let o
      if (typeof create[name] === 'function') {
        o = ((create as Record<string, () => unknown>)[name])()
        assert.throws(() => {
          new ((gdal as Record<string, unknown>)[name] as new () => unknown)()
        }, /Cannot create .* directly|doesnt have a constructor|abstract/)
      } else if (Array.isArray(create[name])) {
        // This is a technique for calling apply on the new operator, it is ugly enough in JS, but TS transforms it into a modern art piece
        o = new (Function.prototype.bind.apply(((gdal as Record<string, unknown>)[name] as new () => unknown), [ null, ...(create[name] as unknown[]) ]))()
      }
      assert.instanceOf(o, ((gdal as Record<string, unknown>)[name] as new () => unknown))
      assert.match(((gdal as Record<string, unknown>)[name] as new () => unknown).prototype.toString.call(o), new RegExp(name))
      assert.throws(() => {
        ((gdal as Record<string, unknown>)[name] as () => unknown)()
      }, /Cannot call constructor|abstract/)
    })
  }
})
