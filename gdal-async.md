This [node-gdal-async](https://github.com/mmomtchev/node-gdal-async) binding for
Node.js provides a feature-complete way of reading, writing, and manipulating
geospatial data, raster and vector, synchronously and asynchronously using [GDAL](http://www.gdal.org/).

```
// sample: open a shapefile and display all features as geojson
const dataset = gdal.open("states.shp");

dataset.layers.get(0).features.forEach(function(feature) {
    console.log(feature.getGeometry().toJSON());
});

// same thing, but asynchronously, allowing integration in server-side multi-user code
const dataset = await gdal.openAsync("states.shp");
const features = (await dataset.layers.getAsync(0)).features;
const len = await features.countAsync();

for (let i = t; i < len; i++) {
  features.getAsync(i, (error, feature) => {
    console.log(feature.getGeometry().toJSON());
  });
}
```

See `ASYNCIO.md` for some performance considerations when designing code
that will run multiple parallel operations on the same dataset.

<iframe
src="http://ghbtns.com/github-btn.html?user=mmomtchev&repo=node-gdal-async&type=watch&count=true"
allowtransparency="true" frameborder="0" scrolling="0" width="90" height="20"
style="margin-top:20px"></iframe> <iframe
src="http://ghbtns.com/github-btn.html?user=mmomtchev&repo=node-gdal-async&type=fork&count=true"
allowtransparency="true" frameborder="0" scrolling="0" width="200"
height="20"></iframe>
