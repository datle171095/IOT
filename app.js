var express = require('express');
var http = require('http');
var app = express();
var myController = require('./controller/myController');
var mqtt = require('mqtt');
var mongoose = require('mongoose');
const EventEmitter = require('events');
const emitter = new EventEmitter();
emitter.setMaxListeners(20);
//Setting view engine
app.set('view engine', 'ejs');
//Handler static file
app.use(express.static('views'));
//Fire my controller
myController(app, mqtt, mongoose);
//Listen on port 3000
var httpServer = http.createServer(app);
httpServer.listen(3000, '127.0.0.1');