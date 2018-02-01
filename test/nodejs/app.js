var http = require('http');

var server = http.createServer(function (req, res) {
  // res.writeHead(200, { 'Content-Type' : 'text/plain' });
  // res.end('Hello World');
  res.sendFile('/app/index.html');
});

server.listen(8000);
