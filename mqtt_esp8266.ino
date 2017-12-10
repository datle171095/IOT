#include <ESP8266WiFi.h> //ESP8266 Library
#include <PubSubClient.h> //MQTT Library

#define SensorPin A0 //Set the sensor input pin
#define ledPin D0 //Set the indicator LED pin
#define Offset 0.00 //Offset value 
#define minPhValue 5.5  // Min Ph value 
#define maxPhValue 7.0  // Max Ph value
#define pHPumpPin D5 //Set the Ph Pump pin
#define pumpPin D6  //Set the Water Pump pin
#define ArrayLenth  40 //Number of sample
int pHArray[ArrayLenth]; //Array for sampling sensor value
int pHArrayIndex=0; 
float pHValue, voltage; 
const unsigned long samplingInterval = 50; //Time between sampling
const unsigned long printInterval = 2000; //The interval of the message send to broker
const unsigned long autoInterval = 2000; //Delay for automatic() function
const unsigned long onTime = 2000; //Ph pump ontime interval
const unsigned long offTime = 4000; //Ph pump offtime interval
bool phPumpState = 1; //State of the Ph pump (1/0)
unsigned long phPumpInterval = offTime; //Pump interval

const char* ssid = "IoT-TLU Lab"; //Wifi SSID
const char* password = "@iottlu2018"; //Wifi Password
const char* mqtt_server = "172.16.6.10"; //Mqtt address

WiFiClient espClient;
PubSubClient client(espClient);

//Run the code once 
void setup() {
  Serial.begin(115200); 
  client.setServer(mqtt_server, 1883); 
  client.setCallback(callback); //
  setup_wifi();
  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(pHPumpPin,OUTPUT);
  digitalWrite(pHPumpPin, HIGH);
  pinMode(pumpPin,OUTPUT);
  digitalWrite(pumpPin, LOW);
}

//Connect to wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Handle message arrived
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String str = topic;

  if(str == "datle/action/auto"){
     if ((char)payload[0] == '1') {
      digitalWrite(ledPin, HIGH);
      client.subscribe("datle/action/pin1");
      client.subscribe("datle/action/pin2");
      client.publish("datle/confirm", "autooff");
     } else if ((char)payload[0] == '0'){
      digitalWrite(ledPin, LOW);
      client.unsubscribe("datle/action/pin1");
      client.unsubscribe("datle/action/pin2");
      client.publish("datle/confirm", "autoon");
    }
  }
  else if(str == "datle/action/pin1"){
     if ((char)payload[0] == '1') {
      digitalWrite(pHPumpPin, HIGH);
      client.publish("datle/confirm", "pin1off");
     } else if ((char)payload[0] == '0'){
      digitalWrite(pHPumpPin, LOW);
      client.publish("datle/confirm", "pin1on");
    }
  }
  else if(str == "datle/action/pin2"){
     if ((char)payload[0] == '1') {
      digitalWrite(pumpPin, HIGH);
      client.publish("datle/confirm", "pin2off");
     } else if ((char)payload[0] == '0'){
      digitalWrite(pumpPin, LOW);
      client.publish("datle/confirm", "pin2on");
    }
  }
}

//Reconnecting to MQTT server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe("datle/action/auto");
      if(digitalRead(ledPin)) {
        client.subscribe("datle/action/pin1");
        client.subscribe("datle/action/pin2");
      }else{
        client.unsubscribe("datle/action/pin1");
        client.unsubscribe("datle/action/pin2");
      }
      Status(); //Send status of digital pin to MQTT server
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//Send status of DG pin to MQTT server
void Status(){
  if(digitalRead(ledPin))
    client.publish("datle/confirm", "autooff");
  else
    client.publish("datle/confirm", "autoon");

  if(digitalRead(pHPumpPin))
    client.publish("datle/confirm", "pin1off");
  else
    client.publish("datle/confirm", "pin1on");

  if(digitalRead(pumpPin))
    client.publish("datle/confirm", "pin2off");
  else
    client.publish("datle/confirm", "pin2on");
}

void loop() {
  if (!client.connected()) reconnect();
  printPhValue();
  Automatic();
  client.loop();
}

//Get the voltage value
float getVoltageValue(){
  pHArray[pHArrayIndex++]=analogRead(SensorPin);
  if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
  return float(avergearray(pHArray, ArrayLenth)*5.0/1024);
}

//And the Ph value
float getPhValue(){
  return float(3.5*getVoltageValue()+Offset);
}

//Print Ph value to serial and publish to broker
void printPhValue(){
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  if(millis()-samplingTime > samplingInterval){
    voltage = getVoltageValue();
    pHValue = 3.5*voltage+Offset;
    samplingTime = millis();
  }
  if(millis() - printTime > printInterval){
    //Serial.print("Voltage:");
   // Serial.print(voltage,2);
    Serial.print("    pH value: ");
    Serial.println(pHValue,2);
    String a = String(pHValue,2);
    client.publish("datle/sensor", a.c_str());
    printTime=millis();
  }
}

//This function handle auto mode
void Automatic(){
  static unsigned long autoTime = millis(); 
  static unsigned long pHPumpTime = millis(); 
  if(!digitalRead(ledPin)){
    if(digitalRead(pumpPin)){
      digitalWrite(pumpPin, LOW);
      client.publish("datle/confirm", "pin2on");
    }
      
    if((millis()-autoTime) > autoInterval){
      if(pHValue>=minPhValue && pHValue<=maxPhValue){
        bool temp = digitalRead(pHPumpPin);
        digitalWrite(pHPumpPin,HIGH);
        if(!temp)
          client.publish("datle/confirm", "pin1off");
        pHPumpTime = millis();
      }
      else if(pHValue>maxPhValue){
          if ((millis() - pHPumpTime) > phPumpInterval) {
            if (phPumpState) {
              phPumpInterval = onTime;
            } else {
              phPumpInterval = offTime;
            }
            phPumpState = !(phPumpState);
            digitalWrite(pHPumpPin,phPumpState);
            if(digitalRead(pHPumpPin))
              client.publish("datle/confirm", "pin1off");
            else
              client.publish("datle/confirm", "pin1on");
            pHPumpTime =  millis();
          }
      }
    autoTime = millis();
    }
  } else {
    autoTime = millis();
    pHPumpTime = millis();
  }
}

//calulate the averge value
double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
