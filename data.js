$(document).ready(function(){
    setInterval(function(){
      $.ajax({
        url: "/",
        type: "POST",
        //dataType: 'json',
        success: function(result){
          //$("#demo").text(result);
          console.log(result);
          google.charts.load('current', {packages: ['corechart', 'line']});
          google.charts.setOnLoadCallback(drawBasic);

          function drawBasic() {
          var data = new google.visualization.DataTable();
          data.addColumn('number', 'X');
          data.addColumn('number', 'PH');
          data.addRows(result);
          var options = {
            hAxis: {
              title: 'Time(s)'
            },
            vAxis: {
              title: 'PH'
            }
          };
          var chart = new google.visualization.LineChart(document.getElementById('chart_div'));
          chart.draw(data, options);
          }
          $('#ph').html('<p><strong>PH: ' + result[result.length-1][1].toString() + '</strong></p>');
        }
      });
    },2000);

    client = new Paho.MQTT.Client("ws://192.168.0.109:8080/", "clientId");
    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;
    //client.onMessageDelivered = onMessageDelivered;
    client.connect({onSuccess:onConnect});
    function onConnect() {
      console.log("onConnect");
      client.subscribe("datle/confirm");
    }
    function onConnectionLost(responseObject) {
      if (responseObject.errorCode !== 0) {
        console.log("onConnectionLost:"+responseObject.errorMessage);
      }
    }

    function onMessageArrived(message) {
      console.log(message.payloadString);
      if(message.payloadString === 'pin1on')
        $('#pin1').text('Ph pump is on')
      else if(message.payloadString ==='pin1off')
        $('#pin1').text('Ph pump is off')

      if(message.payloadString === 'pin2on')
        $('#pin2').text('Water pump is on')
      else if(message.payloadString ==='pin2off')
        $('#pin2').text('Water pump is off')

      if(message.payloadString === 'autoon') {
        $('#auto').text('Auto is on')
        $('#pin1-on, #pin1-off').addClass('disabled');
      }
      else if(message.payloadString ==='autooff') {
        $('#auto').text('Auto is off')
        $('#pin1-on, #pin1-off').removeClass('disabled');
      }
    }

    $('#pin1-on').click(function(){
      var message = new Paho.MQTT.Message('0');
      message.destinationName = "datle/action/pin1";
      client.send(message);
    });

    $('#pin1-off').click(function(){
      var message = new Paho.MQTT.Message('1');
      message.destinationName = "datle/action/pin1";
      client.send(message);
    });
    $('#pin2-on').click(function(){
      var message = new Paho.MQTT.Message('0');
      message.destinationName = "datle/action/pin2";
      client.send(message);
    });

    $('#pin2-off').click(function(){
      var message = new Paho.MQTT.Message('1');
      message.destinationName = "datle/action/pin2";
      client.send(message);
    });
    $('#auto-on').click(function(){
      var message = new Paho.MQTT.Message('0');
      message.destinationName = "datle/action/auto";
      client.send(message);
    });

    $('#auto-off').click(function(){
      var message = new Paho.MQTT.Message('1');
      message.destinationName = "datle/action/auto";
      client.send(message);
    });
});
