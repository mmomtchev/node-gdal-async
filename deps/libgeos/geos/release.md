2024-09-06

- New things:
  - Add Angle::sinCosSnap to avoid small errors, e.g. with buffer operations (GH-978, Mike Taves)
  - Add classes for curved geometry types: CircularString, CompoundCurve, CurvedPolygon, MultiCurve,
    MultiSurface (GH-1046, Dan Baston/German QGIS users group/Canton of Basel-Landschaft/Canton of Zug)
  - Support curved geometry types in WKT/WKB readers/writers (GH-1046, GH-1104, GH-1106, Dan Baston)
  - 3D read and write support for GeoJSON (GH-1150, Oreilles)
  - Port of RelateNG https://github.com/locationtech/jts/pull/1052 (Martin Davis, Paul Ramsey)
    - Rewrite of boolean predicates and relate matrix calculations
    - "Prepared" mode now available for all predicates and relate matrix
    - CAPI functions GEOSPreparedRelate and GEOSPreparedRelatePattern expose new functionality
    - CAPI implementations of GEOSPreparedTouches, etc, that were previously defaulting 
      into non-prepared implementations now default into the RelateNG prepared implementation
    - Prepared implementations for Intersects, Covers, still use the older implementations
    - https://lin-ear-th-inking.blogspot.com/2024/05/jts-topological-relationships-next.html
    - https://lin-ear-th-inking.blogspot.com/2024/05/relateng-performance.html 

- Breaking Changes
  - Zero-length linestrings (eg LINESTRING(1 1, 1 1)) are now treated as equivalent to points (POINT(1 1)) in boolean predicates
  - CMake 3.15 or later is requried (GH-1143, Mike Taves)

- Fixes/Improvements:
  - WKTReader: Points with all-NaN coordinates are not considered empty anymore (GH-927, Casper van der Wel)
  - WKTWriter: Points with all-NaN coordinates are written as such (GH-927, Casper van der Wel)
  - ConvexHull: Performance improvement for larger geometries (JTS-985, Martin Davis)
  - Distance: Improve performance, especially for point-point distance (GH-1067, Dan Baston)
  - Intersection: change to using DoubleDouble computation to improve robustness (GH-937, Martin Davis)
  - Fix LargestEmptyCircle to respect polygonal obstacles (GH-939, Martin Davis)
  - Fix WKTWriter to emit EMPTY elements in multi-geometries (GH-952, Mike Taves)
  - Fix IncrementalDelaunayTriangulator to ensure triangulation boundary is convex (GH-953, Martin Davis)
  - Fix PreparedLineStringDistance for lines within envelope and polygons (GH-959, Martin Davis)
  - Improve scale handling for PrecisionModel (GH-956, Martin Davis)
  - Fix error in CoordinateSequence::add when disallowing repeated points (GH-963, Dan Baston)
  - Fix WKTWriter::writeTrimmedNumber for big and small values (GH-973, Mike Taves)
  - Fix InteriorPointPoint to handle empty elements (GH-977, Martin Davis)
  - Fix TopologyPreservingSimplifier endpoint handling to avoid self-intersections (GH-986, Martin Davis)
  - Fix spatial predicates for MultiPoint with EMPTY (GH-989, Martin Davis)
  - Fix DiscreteHausdorffDistance for LinearRing (GH-1000, Martin Davis)
  - Fix IsSimpleOp for MultiPoint with empty element (GH-1005, Martin Davis)
  - Fix PreparedPolygonContains for GC with MultiPoint (GH-1008, Martin Davis)
  - Fix reading WKT with EMPTY token with white space (GH-1025, Mike Taves)
  - Fix buffer Inverted Ring Removal check (GH-1056, Martin Davis)
  - Add PointLocation.isOnSegment and remove LineIntersector point methods (GH-1083, Martin Davis)
  - Densify: Interpolate Z coordinates (GH-1094)
  - GEOSLineSubstring: Fix crash on NaN length fractions (GH-1088, Dan Baston)
  - MinimumClearance: Fix crash on NaN inputs (GH-1082, Dan Baston)
  - Centroid: Fix crash on polygons with empty holes (GH-1075, Dan Baston)
  - GEOSRelatePatternMatch: Fix crash on invalid DE-9IM pattern (GH-1089, Dan Baston)
  - CoveragePolygonValidator: add section performance optimization (GH-1099, Martin Davis)
  - TopologyPreservingSimplifier: fix to remove ring endpoints safely (GH-1110, Martin Davis)
  - TopologyPreservingSimplifier: fix stack overflow on degenerate inputs (GH-1113, Dan Baston)
  - DouglasPeuckerSimplifier: fix stack overflow on NaN tolerance (GH-1114, Dan Baston)
  - GEOSConcaveHullOfPolygons: Avoid crash on zero-area input (GH-1076, Dan Baston) 

