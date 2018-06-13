const example = require('bindings')('example')

const x = 1
const y = 41

example.add(x, y, (err, res) => {
    console.log(res)
})
