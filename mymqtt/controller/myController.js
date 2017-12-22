var bodyParser = require('body-parser');
var urlencodedParser = bodyParser.urlencoded({ extended: false });

module.exports = function(app, mqtt, mongoose){
  var client  = mqtt.connect('mqtt://192.168.0.109:1883');
  const sensorModel = mongoose.model('Sensor', new mongoose.Schema({data: Number}));
  const stateModel = mongoose.model('State', new mongoose.Schema({value: String}));
  mongoose.Promise = global.Promise;
  var option = {
    useMongoClient: true,
    keepAlive: 10000,
    connectTimeoutMS: 30000,
    socketTimeoutMS : 30000,
    autoReconnect: true,
    poolSize: 10,
    ha: true
  };

  //Subcribe topic when connected to the MQTT broker
  client.on('connect', function () {
    client.subscribe('datle/sensor');
    client.subscribe('datle/confirm');
  });
  //Handle message arived
  client.on('message', function (topic, message) {
    //console.log(message.toString());
      if(topic.toString() === 'datle/sensor'){
        //Connect to the db
        var db = mongoose.connect('mongodb://localhost/mydb', option, function(err){
          if (err) return err;
          //if number of collection < 1000 then save its to the db
          sensorModel.count({}, function(err, result){
            if(result < 1000)
              var sensorVal = new sensorModel({data: parseFloat(message)}).save(function(err){
                if (err) return err;
                db.close(function(){
                  console.log('sensor connetion close');
                });
              });
            //if number of collection > 1000 then delete its from the db
            else{
              sensorModel.deleteMany({}, function(err){
                if (err) return err;
                db.close(function(){
                  console.log('sensor connetion close');
                });
              });
            }
          });
        });

      }
      else if (topic.toString() === 'datle/confirm') {
        //Update the state of digital pin to db
        var db = mongoose.connect('mongodb://localhost/mydb', option, function(err){
          if(message.toString() === 'autoon' || message.toString() === 'autooff') {
          stateModel.findOneAndUpdate({value: /(auto).*/},
            {value: message.toString()},
            {upsert: true},
            function(err, doc){
              db.close(function(){
                console.log('confirm close');
              });
            });
          }
          else if(message.toString() === 'pin1on' || message.toString() === 'pin1off') {
            stateModel.findOneAndUpdate({value: /(pin1).*/},
              {value: message.toString()},
              {upsert: true},
              function(err, doc){
                db.close(function(){
                  console.log('confirm close');
                });
              });
          }
          else if(message.toString() === 'pin2on' || message.toString() === 'pin2off') {
            stateModel.findOneAndUpdate({value: /(pin2).*/},
              {value: message.toString()},
              {upsert: true},
              function(err, doc){
                db.close(function(){
                  console.log('confirm close');
                });
              });
           }
        });
      }
    });


  //Handle get request
  app.get('/', function(req, res){
    var db = mongoose.connect('mongodb://localhost/mydb', option, function(err){
      if (err) return err;
      stateModel.find({}, function(err, result){
        if (err) return err;
        res.render('index', {data: result});
        db.close(function(){
          console.log('get close');
        });
      });
    });
  });
  //Handle post request
  app.post('/', urlencodedParser, function(req, res){
    var db = mongoose.connect('mongodb://localhost/mydb', option,function(err){
      if (err) return err;
      //Send the sensor value back to the client
      sensorModel.find({}, function(err, result){
        if (err) return err;
        var time = 0;
        var arr = [];
        for(var i=0; i<result.length;i++){
          arr.push([time, result[i].data]);
          time += 2;
        }
        res.send(arr);
        db.close(function(){
          console.log('post connection close');
        });
      });
    });
  });
};
