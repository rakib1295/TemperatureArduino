#include <FS.h>    
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include "DHT.h"
#define DHTTYPE DHT22


//for LED status
#include <Ticker.h>
Ticker ticker;
ESP8266WebServer server(80);

uint8_t DHTPin = D5;
               
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);   

float Temperature;
float Humidity;

struct ConfigData
{
  String Title = "";
  String Name = "";
  float CriticalTemp = 0.0;
  float HiCriticalHum = 0.0;
  float LowCriticalHum = 0.0;
  int PhnNumberCount = 0;
  String PhnNumbers[10];
} _configdata;

char webpage[] PROGMEM = R"rawliteral(
<html>
<head>
<title>Config Form for Sensor Device</title>
</head>
<body>
<h1 align="middle">Config SMS Settings</h1>
<form align="left"><i>Saving will replace previous data. It will save in file system. It is a sophisticated file system. So, please save carefully, and don't click save button repeatedly.</i>
    <fieldset>
      <div>
        <label for="Title">Title of Portal: </label>      
        <input id="Title" type="text" placeholder="Title">
      </div><br>
      <div>
        <label for="Name">Name of Portal: </label>      
        <input id="Name" type="text" placeholder="Name">
      </div><br>
      <div>
        <label for="CriticalTemp">Critical Temperature: </label>      
        <input id="CriticalTemp" type="number" placeholder="Critical Temperature">°C</input>
      </div><br>
      <div>
        <label for="HiCriticalHum">Higher Critical Humidity: </label>
        <input id="HiCriticalHum" type="number" max='100' min='1' placeholder="value">% (Must be: Lower Critical Humidity < value <= 100)</input>
      </div><br>
    <div>
        <label for="LowCriticalHum">Lower Critical Humidity: </label>
        <input id="LowCriticalHum" type="number" max='100' min='0'  placeholder="value">% (Must be: 0 < value < Higher Critical Humidity)</input>
      </div><br>    
    
    <div>
        <label for="PhnNumberCount">Phone number count: </label>
        <input type="number" id="PhnNumberCount" max=10 min='1' placeholder="Count"> (Maximum 10 phone numbers)</input>  
    <button id="AddPhnNumbers" type="button" onclick="addFields()">Create forms</button>
    <br><br>
    <div id="container"/>
    </div><br> 
    <br>
      <div>
        <button class="primary" id="savebtn" type="button" onclick="myFunction()">SAVE</button>
      </div>
    </fieldset>
  </form><br> <br> <br>
  <div>
    <h2 hidden id="SavedDatahead">Saved configuration data:</h2>
    <p1>
    <div id="SavedData"></div>
    </p1>
</body>
<script>
  function addFields()
  {
    // Number of inputs to create
    var number = document.getElementById("PhnNumberCount").value;
    // Container <div> where dynamic content will be placed
    var container = document.getElementById("container");
    // Clear previous contents of the container
    while (container.hasChildNodes()) {
      container.removeChild(container.lastChild);
    }
    for (i=0;i<number;i++)
    {
      // Append a node with a random text
      container.appendChild(document.createTextNode("Phone Number " + (i+1) + ": "));
      // Create an <input> element, set its type and name attributes
      var input = document.createElement("input");
      input.type = "number";
      input.id = "PhnNumber" + i;
      input.placeholder = "01XXXXXXXXX";
      container.appendChild(input);
      // Append a line break 
      container.appendChild(document.createElement("br"));
    }
  }
  function myFunction()
  {
    console.log("button was clicked!");
    var Title = document.getElementById("Title").value;
    var Name = document.getElementById("Name").value;
    var CriticalTemp = document.getElementById("CriticalTemp").value;
    var HiCriticalHum = document.getElementById("HiCriticalHum").value;
    var LowCriticalHum = document.getElementById("LowCriticalHum").value;
    //var PhnNumbers = document.getElementById("container").value;
    
    var PhnNumberCount = document.getElementById("PhnNumberCount").value;
    //var PhnNumbers = document.getElementById("container");
    var phndata = [];
    for (i=0; i<PhnNumberCount; i++)
    {
      if(document.getElementById("PhnNumber" + i).value != "") //if blank form, then will ignore
      {
        phndata[i] = document.getElementById("PhnNumber" + i).value;
      }
    }      
    
    var data = {Title:Title, Name:Name, CriticalTemp:CriticalTemp, HiCriticalHum:HiCriticalHum, LowCriticalHum:LowCriticalHum, PhnNumberCount:phndata.length, PhnNumbers:phndata};
    console.log(data);
    var xhr = new XMLHttpRequest();
    var url = "/settings";
    xhr.onreadystatechange = function() 
    {
      if (this.readyState == 4 && this.status == 200) 
      {
        // Typical action to be performed when the document is ready:
        if(xhr.responseText != null)
        {
          document.getElementById("SavedData").innerHTML = this.responseText;
      document.getElementById("SavedDatahead").style.visibility = "visible";
        }
      }
    };
    xhr.open("POST", url, true);
    xhr.send(JSON.stringify(data));
  };
</script>
</html>
)rawliteral";

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //server.on();
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

String ShowDataHTML(float Temperaturestat, float Humiditystat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>";
  ptr += _configdata.Title;
  ptr += "</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>";
  ptr += _configdata.Name;
  ptr += "</h1>\n";

  ptr += "<p>Temperature: ";
  ptr += Temperaturestat;
  ptr += "°C</p>"; 
  ptr += "<p>Humidity: ";
  ptr += Humiditystat;
  ptr += "%</p>";

  ptr += "</div><br><br>\n";
  ptr += "<button onclick=\"window.location.href = '/saveddata';\">Show saved configuration data</button>";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

void getData() {

  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity

  Serial.println("Temperature data.. ");
  Serial.println(Temperature);
  
  server.send(200, "text/html", ShowDataHTML(Temperature, Humidity));
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DHTPin, INPUT);
  
  dht.begin();
  SPIFFS.begin();
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  Serial.println("again called");
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //set static ip
  wifiManager.setSTAStaticIPConfig(IPAddress(192, 168, 1, 191), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));

  wifiManager.setConfigPortalTimeout(180);
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Temp_AutoConnect_AP")) {
    Serial.println("failed to connect and hit timeout");
    //delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on if low
  digitalWrite(BUILTIN_LED, LOW);
  Serial.println("now: "+WiFi.SSID());

  //////////////////////////////////////////////////////////end of WIFI part

  ReadFromFS();
  server.on("/config",[](){server.send_P(200,"text/html", webpage);});
  server.on("/settings", HTTP_POST, WriteToFS);
  server.on("/", getData);
  server.on("/saveddata", [](){server.send(200, "text/html", ConfigDataValues());});
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("Server started");

}

void loop(){
  // put your main code here, to run repeatedly:
  server.handleClient();
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void WriteToFS(){
  String data = server.arg("plain");
  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);

  Serial.println("Write Data to config.json file ... \n");
  File configFile = SPIFFS.open("/config.json", "w");
  if(!configFile){
    Serial.println("- failed to open file for writing");
    return;
  }
  jObject.printTo(configFile);  
  Serial.println("config file saved: " + configFile);
  configFile.close();
  
  //server.send(200, "application/json", "{\"status\" : \"ok\"}");
  delay(1000);

  ReadFromFS();
  server.send(200, "text/plain", ConfigDataValues());
}

String ConfigDataValues()
{
  String ptr = "Title of Portal: " + (String)_configdata.Title + "<br>";
  ptr += "Name of Portal: " + (String)_configdata.Name + "<br>";
  ptr += "Critical Temperature: " + (String)_configdata.CriticalTemp + "<br>"; 
  ptr += "Higher Critical Humidity: " + (String)_configdata.HiCriticalHum + "<br>";
  ptr += "Lower Critical Humidity: " + (String)_configdata.LowCriticalHum + "<br>";
  ptr += "Phone Numbers: ";

  for(int i=0; i< _configdata.PhnNumberCount; i++)
  {
    if(i == _configdata.PhnNumberCount - 1)
      ptr += (String)_configdata.PhnNumbers[i];
    else
      ptr += (String)_configdata.PhnNumbers[i] + ", ";
  }
  return ptr;
}

void ReadFromFS()
{
  if(SPIFFS.exists("/config.json"))
  {
    File configFile = SPIFFS.open("/config.json", "r");
    if(configFile)
    {
      Serial.print("Reading Data from Config File.....\n");
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      DynamicJsonBuffer jsonBuffer;
      JsonObject& jObject = jsonBuffer.parseObject(buf.get());
      if(jObject.success())
      {
        const char* ch;
        ch = jObject["Title"];
        _configdata.Title = (String)ch;

        ch = jObject["Name"];
        _configdata.Name = (String)ch;
        _configdata.CriticalTemp = jObject["CriticalTemp"];
        _configdata.HiCriticalHum = jObject["HiCriticalHum"];
        _configdata.LowCriticalHum = jObject["LowCriticalHum"];
        _configdata.PhnNumberCount = jObject["PhnNumberCount"];
        //strcpy(_configdata.PhnNumbers, jObject["PhnNumbers"]);

        
        for(int i=0; i<_configdata.PhnNumberCount; i++)
        {
          ch = jObject["PhnNumbers"][i]; 
          _configdata.PhnNumbers[i] = (String)ch;
          Serial.println(_configdata.PhnNumbers[i]);
      
        }
        
        Serial.println("Config File data: ");
        Serial.println(_configdata.Title);
        Serial.println(_configdata.Name);
        Serial.println(_configdata.CriticalTemp);
        Serial.println(_configdata.HiCriticalHum);
        Serial.println(_configdata.LowCriticalHum);
        Serial.println(_configdata.PhnNumberCount);
        
        delay(1000);
      }
    }
  }
}
