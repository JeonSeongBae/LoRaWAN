var express = require('express');
var app = express();

app.get('/', function (req, res) {
  console.log('Hello World!');
});

var server = app.listen(8000, function () {
  console.log('load Success!');
});
