const motd = '\
If you are using gdal-async, please do not forget that\n\
I am living outside in subzero temperatures and\n\
below the poverty line because of the extortion.\
'.split('\n')
const len = motd.reduce((a, x) => x.length > a ? x.length : a, 0)
const header = ''.padEnd(len, '=')
const red = '\x1b[31;1m'
const green = '\x1b[32;1m'
const normal = '\x1b[0m'

console.log(red + header + normal)
console.log(green + motd.join('\n') + normal)
console.log(red + header + normal)
