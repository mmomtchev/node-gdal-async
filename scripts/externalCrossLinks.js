const types = {
  Promise: 'https://developer.mozilla.org/en-US/docs/Web/API/Promise',
  TypedArray: 'https://developer.mozilla.org/en-US/docs/Web/API/ArrayBufferView',
  Error: 'https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error'
}

module.exports = function externalCrossLinks(item) {
  if (types[item]) {
    return types[item]
  }
  return undefined
}
