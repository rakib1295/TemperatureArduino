#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"

// Uncomment one of the lines below for whatever DHT sensor type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
String API_KEY = "AFH6WK3DZZ45Z8JF";
IPAddress ip(192, 168, 1, 191);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);    
IPAddress dns(8, 8, 8, 8);    

const char* ssid     = "dlink";
const char* password = "sbnbtcl321456";

ESP8266WebServer server(80);

// DHT Sensor
uint8_t DHTPin = D5;
               
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);                

float Temperature;
float Humidity;
  HTTPClient http;
void setup() {
  Serial.begin(115200);
  delay(100);
  
  pinMode(DHTPin, INPUT);

  dht.begin();              

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.config(ip, dns, gateway, subnet);
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/service", handle_OnConnect_Service);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

}
int timeSinceLastRead = 0;
int interval = 300000;
unsigned long previousMillis = 0;
unsigned long currentMillis;

void loop() {
  HTTPClient http;

  
  server.handleClient();
       currentMillis = millis();
timeSinceLastRead = currentMillis - previousMillis;
 if (timeSinceLastRead >= interval) {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      previousMillis = currentMillis;
      return;
    }
   String APILink = "http://api.thingspeak.com/update?api_key=W6QM23O1K5GEV9DH&field1=" + String(t) + "&field2=" + String(h);
   //String humidityLink = "http://api.thingspeak.com/update?api_key=PPGQ19KDL4Z4FTU6&field1=" + String(h);
Serial.println(APILink);

  http.begin(APILink);
  int httpCodeT = http.GET();
  Serial.println(httpCodeT);
  http.end();

//  delay(1000);
//
//  Serial.println(humidityLink);
//  
// http.begin(humidityLink);
// int httpCodeH = http.GET();
// Serial.println(httpCodeH);
// http.end();
 
  
    

    timeSinceLastRead = 0;
    previousMillis = currentMillis;
 
  
}
}

void handle_OnConnect() {

 Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity 
  server.send(200, "text/html", SendHTML(Temperature,Humidity)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat,float Humiditystat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP8266 Weather Report</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>ESP8266 NodeMCU Weather Report</h1>\n";
  
  ptr +="<p>Temperature: ";
  ptr +=Temperaturestat;
  ptr +="°C</p>";
  ptr +="<p>Humidity: ";
  ptr +=Humiditystat;
  ptr +="%</p>";
  
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}


void handle_OnConnect_Service() {

 Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity 
  server.send(200, "text/plain", SendJSON(Temperature,Humidity)); 
}


String SendJSON(float Temperaturestat,float Humiditystat){
  String ptr = "{\"channel\":{\"id\":0,\"name\":\"NA\",\"description\":\"NA\",\"latitude\":\"0.0\",\"longitude\":\"0.0\",\"field1\":\"Temperature\",\"field2\":\"Humidity\",\"created_at\":\"NA\",\"updated_at\":\"NA\",\"last_entry_id\":0},\"feeds\":[{\"created_at\":\"NA\",\"entry_id\":0,\"field1\":";
  ptr += Temperaturestat;
  ptr += ",\"field2\":";
  ptr += Humiditystat;
  ptr += "}]}";
  return ptr;
}
