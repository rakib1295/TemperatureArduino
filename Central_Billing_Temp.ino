#include <FS.h>    
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <HttpClient.h>
#include <ArduinoOTA.h>  //-------Comment if On The Air is not needed---------
#define DHTTYPE DHT22


//for LED status
#include <Ticker.h>
Ticker ticker;
ESP8266WebServer server(80);

uint8_t DHTPin = D5; //--------------************-----------see carefully the pin number for sensor-------------*******-------------------
uint8_t PIN_AP = 0;
               
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

String CloudLink = "http://api.thingspeak.com/update?api_key=W6QM23O1K5GEV9DH";
String BaseLink = "http://bulksms.teletalk.com.bd/link_sms_send.php?op=SMS&user=temperature&pass=Temp$201920&mobile=";
bool SMSRunning = false;
bool SMSStopped = false;

HTTPClient http;
const char* www_realm = "Custom Auth Realm";
// the Content of the HTML response in case of Unautherized Access Default:empty
String authFailResponse = "Authentication Failed";

struct ConfigData
{
  String PortalTitle = "";
  String PortalName = "";
  String PlaceName = "";
  float CriticalTemp = 0.0;
  float HiCriticalHum = 0.0;
  float LowCriticalHum = 0.0;
  int PhnNumberCount = 0;
  int SensePeriod = 0; //min
  int SMSInterval = 0; //min
  String PhnNumbers[10];
} _configdata;

struct IPConfigData
{
  IPAddress IP = IPAddress(192, 168, 0, 191);
  IPAddress Gateway = IPAddress(192, 168, 0, 1);
  IPAddress Subnet = IPAddress(255, 255, 255, 0);
} _IPconfigdata;

const char WebPage_Header[] PROGMEM = R"rawliteral(<!DOCTYPE html> 
<html> 
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0  user-scalable=no"> 
<title>)rawliteral";

const char WebPage_Style[] PROGMEM = R"rawliteral(</title> 
  <style>
  html
  {
    font-family: Times New Roman; display: inline-block; margin: 0px auto;
  } 
  body
  {margin-top: 20px;}
  h1 {color: Green; margin: 50px auto 30px;}
  h2 {color: DeepSkyBlue; margin: auto;}
  p {font-size: 24px; margin-bottom: 10px;} 
  footer
  {
  position:fixed; font-size:14px; left: 0; bottom: 10px; width: 100%; color:DeepSkyBlue; text-align: middle;
  } 
  .column
  {
  float: left;
  width: 33.33%;
  } 
  a{color:DarkCyan;}
  
  @media only screen and (max-width: 768px) 
  { [class*="col"] 
    {
      width: 100%;
    }
    footer
    {
      position:relative;
    }
  }
  </style> 
</head> 
<body> 
<div id="webpage" align="center"> 
  <h2>)rawliteral";
  
const char WebPage_P1[] PROGMEM = R"rawliteral(</h2>  <h1>)rawliteral";
const char WebPage_P2[] PROGMEM = R"rawliteral(</h1><p>Temperature: )rawliteral";
const char WebPage_P3[] PROGMEM = R"rawliteral(&deg;C</p><br>
<iframe width="450" height="260" style="border: 1px solid #cccccc;" src="https://thingspeak.com/channels/854795/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&type=line&update=15"></iframe>
<br><br><p>Humidity: )rawliteral";
const char WebPage_P4[] PROGMEM = R"rawliteral(%</p><br>
<iframe width="450" height="260" style="border: 1px solid #cccccc;" src="https://thingspeak.com/channels/854795/charts/2?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&type=line&update=15"></iframe>
<br> )rawliteral";
const char WebPage_Btn1[] PROGMEM = R"rawliteral(<div align="left" style="color:Crimson"><p id="SMSinfo">SMS Running</p> <button onclick="myFunction()">Stop SMS Sending</button></div>)rawliteral";
const char WebPage_Btn23[] PROGMEM = R"rawliteral(<br><div>
 <div>
<form align="left" action="/savedconfig" target="_blank">
<input type="submit"  value="Show saved configuration data"/>
</form><br>
<form align="left" action="/config" target="_blank">
<input type="submit"  value="Edit Configuration"/>
</form>
<br><br>
</div></div>
</div>)rawliteral";

const char WebPage_Footer[] PROGMEM = R"rawliteral(<footer>
<hr>
<div>
  <div class="column" align="left" >
    <p style="font-size:16px"> Developed by:</p>
  <ul>
    <li><a type="button" href="https://www.linkedin.com/in/md-rakib-subaid/" target="_blank">Md. Rakib Subaid</a></li>
    <li><a type="button" href="https://www.linkedin.com/in/mosheyur-rahman-zebin-1a269833/" target="_blank">Mosheyur Rahman</a></li>
    <li><a type="button" href="https://www.linkedin.com/in/mnsagor/" target="_blank">Md. Moniruzzaman Sagor</a></li>
    </ul>
  </div>
  <div class="column" align="center"><br><br>BTCL &copy; 2019 All Rights Reserved<br>Version: 2.2</div>
</div>
</footer>
</body>)rawliteral";

const char WebPage_Script[] PROGMEM = R"rawliteral(<script>
function myFunction()
  {
    var xhr = new XMLHttpRequest();
    var url = "/stopsms";
    xhr.onreadystatechange = function() 
    {
      if (this.readyState == 4 && this.status == 200)
      {
        // Typical action to be performed when the document is ready:
        if(xhr.responseText != null)
        {
          document.getElementById("SMSinfo").innerHTML = this.responseText;
        }
      }
    };
    xhr.open("GET", url, true);
    xhr.send();
  };
</script>)rawliteral";

const char WebPage_End[] PROGMEM = R"rawliteral(</html>)rawliteral";

const char ConfigPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html> 
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0  user-scalable=no"> 
<title>Config Form for Sensor Device</title>
<style>
  html
  {
    font-family: Times New Roman; display: inline-block; margin: 0px auto;
  }
  </style> 
</head>
<body>
<h1 align="middle" style="color:Green;">Configure SMS Settings</h1>
<form align="left"><div style="color:Crimson; font-size: 18px;"><i>Saving will replace previous data. It will save in file system. It is a sophisticated file system. So, please save carefully, and don't click save button repeatedly.</i></div>
    <fieldset>
      <div>
        <button class="primary" id="defaultbtn" type="button" onclick="dValueFunction()">Show Default Values</button>
      </div><br> 
      <div style="color:Green; font-size: 18px;"><i>Edit fields below if needed:</i></div><br><br>
      <div>
        <label for="PortalTitle">Title of Web Portal: </label>      
        <input id="PortalTitle" type="text" placeholder="Title">
      </div><br>
      <div>
        <label for="PortalName">Name of Web Portal: </label>
        <input id="PortalName" type="text" placeholder="Portal Name">
      </div><br>
      <div>
        <label for="PlaceName">Name of Place or Room, where device established: </label>      
        <input id="PlaceName" type="text" placeholder="Place Name">
      </div><br>
      <div>
        <label for="CriticalTemp">Critical Temperature: </label>      
        <input id="CriticalTemp" type="number" max='100' min='0' placeholder="value">&deg;C</input>
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
        <label for="SensePeriod">Sensing period of critical values before sending SMS: </label>
        <input id="SensePeriod" type="number" min='1' max = '100'  placeholder="value">Minute(s) (Must be: 1 <= value < SMS Interval)</input>
      </div><br> 
      
      <div>
        <label for="SMSInterval">SMS Interval: </label>
        <input id="SMSInterval" type="number" min='1' max = '1000' placeholder="value">Minutes (Must be: value > Sensing period)</input>
      </div><br> 
      
    <div>
        <label for="PhnNumberCount">Phone number count: </label>
        <input type="number" id="PhnNumberCount" max=10 min='1' placeholder="Count"> (Maximum 10 phone numbers)</input>  
    <button id="AddPhnNumbers" type="button" onclick="addFields()">Create forms</button>
    <br><br>
    <div id="container"/>
    </div> 
    <br> 
    <br>
      <div>
        <button class="primary" id="savebtn" type="button" onclick="saveFunction()">SAVE</button>
      </div>
    </fieldset>
  </form><br> <br>
  <div>
    <p1>
    <div id="SavedConfig"></div>
    </p1>
</body>
<script>
  function dValueFunction() {
    document.getElementById("PortalTitle").value = "Weather Report";
    document.getElementById("PortalName").value = "Temperature & Humidity Alert System";
    document.getElementById("PlaceName").value = "BTCL Server Room";
    document.getElementById("CriticalTemp").value = "27";
    document.getElementById("HiCriticalHum").value = "80";
    document.getElementById("LowCriticalHum").value = "30";
    document.getElementById("SensePeriod").value = "5";
    document.getElementById("SMSInterval").value = "30";
  }
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
  function saveFunction()
  {
    console.log("button clicked!");
    var PortalTitle = document.getElementById("PortalTitle").value;
    var PortalName = document.getElementById("PortalName").value;
  var PlaceName = document.getElementById("PlaceName").value;
    var CriticalTemp = document.getElementById("CriticalTemp").value;
    var HiCriticalHum = document.getElementById("HiCriticalHum").value;
    var LowCriticalHum = document.getElementById("LowCriticalHum").value;
    var SensePeriod = document.getElementById("SensePeriod").value;
    var SMSInterval = document.getElementById("SMSInterval").value;
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
    
    var data = {PortalTitle:PortalTitle, PortalName:PortalName, PlaceName:PlaceName, CriticalTemp:CriticalTemp, HiCriticalHum:HiCriticalHum, LowCriticalHum:LowCriticalHum, SensePeriod:SensePeriod, SMSInterval:SMSInterval, PhnNumberCount:phndata.length, PhnNumbers:phndata};
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
          document.getElementById("SavedConfig").innerHTML = this.responseText;
        }
      }
    };
    xhr.open("POST", url, true);
    //console.log("Stringify: " + JSON.stringify(data));
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

String IPtoString(IPAddress ip)
{
  return String(ip[0]) + String(".") + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
}

IPAddress StringtoIP(String _ip)
{
  uint8_t Parts[4] = {0,0,0,0};
  int Part = 0;
  for ( int i=0; i<_ip.length(); i++ )
  {
    char c = _ip[i];
    if ( c == '.' )
    {
      Part++;
      continue;
    }
    Parts[Part] *= 10;
    Parts[Part] += c - '0';
  }

  return IPAddress( Parts[0], Parts[1], Parts[2], Parts[3]);
}

bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(DHTPin, INPUT);
  
  dht.begin();
  SPIFFS.begin();
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  WiFiManager wifiManager;
  //Local intialization. Once its business is done, there is no need to keep it around
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //wifiManager.setSTAStaticIPConfig(IPAddress(192, 168, 0, 191), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));
  //set static ip
  if(SPIFFS.exists("/ipconfig.json"))
  {
    Serial.println("IP file exist");
    File configFile = SPIFFS.open("/ipconfig.json", "r");
    Serial.println("configFile" + String(configFile));
    if(configFile)
    {
      Serial.print("Reading Data from ipConfig File.....\n");
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      DynamicJsonBuffer jsonBuffer;
      JsonObject& jObject = jsonBuffer.parseObject(buf.get());
      if(jObject.success())
      {
        const char* ch;
        ch = jObject["IP"];
        _IPconfigdata.IP = StringtoIP(String(ch));   
        
        ch = jObject["Gateway"];
        _IPconfigdata.Gateway = StringtoIP(String(ch));

        ch = jObject["Subnet"];
        _IPconfigdata.Subnet = StringtoIP(String(ch));

        Serial.println(_IPconfigdata.IP);
        Serial.println(_IPconfigdata.Gateway);
        Serial.println(_IPconfigdata.Subnet);

        
        delay(1000);
      }
    }
  }

  wifiManager.setSTAStaticIPConfig(_IPconfigdata.IP, _IPconfigdata.Gateway, _IPconfigdata.Subnet);

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

  if(shouldSaveConfig)
  {
    shouldSaveConfig = false;
    Serial.println("Saving IP config:");
    _IPconfigdata.IP =  WiFi.localIP();
    String _ip = IPtoString(_IPconfigdata.IP);
    Serial.println(_ip);
    _IPconfigdata.Gateway =  WiFi.gatewayIP();
    String _gw = IPtoString(_IPconfigdata.Gateway);
    Serial.println(_gw);
    _IPconfigdata.Subnet =  WiFi.subnetMask();
    String _sn = IPtoString(_IPconfigdata.Subnet);
    Serial.println(_sn);

    String data = "{\"IP\":" + _ip + ", \"Gateway\":" + _gw + ", \"Subnet\":" + _sn + "}";

    WriteToFS("IP", data);
  }
    
  //////////////////////////////////////////////////////////end of WIFI part

  ReadFromFS();//////needed to comment sometimes

  //-------Comment if On The Air is not needed---------
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  //ArduinoOTA.setHostname("admin");
  // No authentication by default
   ArduinoOTA.setPassword("Admin1123");
  ArduinoOTA.begin();

  
  server.on("/config",[](){server.send_P(200,"text/html", ConfigPage);});
  server.on("/", getData);
  server.on("/stopsms", HTTP_GET, []() 
  {
    if (!server.authenticate("admin", WiFi.psk().c_str()))
    {
      return server.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
    }
    Serial.println("Server auth ok");

    StopSMS();
  });
  
  server.on("/savedconfig", [](){server.send(200, "text/html", ConfigDataValues());});
  server.onNotFound(handle_NotFound);
  server.on("/settings", HTTP_POST, []() 
  {
    if (!server.authenticate("admin", WiFi.psk().c_str()))
      //Basic Auth Method with Custom realm and Failure Response
      //return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
      //Digest Auth Method with realm="Login Required" and empty Failure Response
      //return server.requestAuthentication(DIGEST_AUTH);
      //Digest Auth Method with Custom realm and empty Failure Response
      //return server.requestAuthentication(DIGEST_AUTH, www_realm);
      //Digest Auth Method with Custom realm and Failure Response
    {
      return server.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
    }
    Serial.println("Server auth ok");

    WriteToFS("SETTINGS", "");
  });

  server.begin();
  Serial.println("Server started");

}

void StopSMS() 
{
  SMSStopped = true;
  SMSRunning = false;
  Serial.println("SMS Stopped Manually.");
  server.send(200, "text/plain", "SMS Stopped");
}


String ShowDataHTML(float Temperaturestat, float Humiditystat)
{
  String ptr = FPSTR(WebPage_Header);
  ptr += _configdata.PortalTitle;
  ptr += FPSTR(WebPage_Style);
  ptr += _configdata.PortalName;
  ptr += FPSTR(WebPage_P1);
    ptr += _configdata.PlaceName;
  ptr += FPSTR(WebPage_P2);
  ptr += String(Temperaturestat);
  ptr += FPSTR(WebPage_P3);
  ptr += String(Humiditystat);
  ptr += FPSTR(WebPage_P4);

  if(SMSRunning)
  {
    ptr += FPSTR(WebPage_Btn1);
  }
  ptr += FPSTR(WebPage_Btn23);
  ptr += FPSTR(WebPage_Footer);
  if(SMSRunning)
  {
    ptr += FPSTR(WebPage_Script);
  }
  ptr += FPSTR(WebPage_End);
  return ptr;
}

void getData() 
{

  float Temperature = dht.readTemperature(); // Gets the values of the temperature
  float Humidity = dht.readHumidity(); // Gets the values of the humidity

  Serial.println("Sensor data.. ");
  Serial.println(Temperature);
  Serial.println(Humidity);
  server.send(200, "text/html", ShowDataHTML(Temperature, Humidity));
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void WriteToFS(String type, String data)
{
  if(type == "SETTINGS")
  {
      data = server.arg("plain");
  }

  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);

  Serial.println("Write Data to json file ... \n");

  File configFile;
  if(type == "SETTINGS")
  {
      configFile = SPIFFS.open("/config.json", "w");
  }
  else
  {
    configFile = SPIFFS.open("/ipconfig.json", "w");
  }
  
  if(!configFile){
    Serial.println("- failed to open file for writing");
    return;
  }
  jObject.printTo(configFile);
  //Serial.println("config file saved: " + configFile);
  configFile.close();
  
  //server.send(200, "application/json", "{\"status\" : \"ok\"}");
  delay(1000);

  if(type == "SETTINGS")
  {
    ReadFromFS();
    server.send(200, "text/plain", ConfigDataValues());
  }
}

String ConfigDataValues()
{
  String ptr = "Here the saved configuration data:<br><br>"; 
  ptr += "Title of Web Portal: " + (String)_configdata.PortalTitle + "<br>";
  ptr += "Name of Web Portal: " + (String)_configdata.PortalName + "<br>";
  ptr += "Name of Place, where device established: " + (String)_configdata.PlaceName + "<br>";
  ptr += "Critical Temperature: " + (String)_configdata.CriticalTemp + "&deg;C<br>"; 
  ptr += "Higher Critical Humidity: " + (String)_configdata.HiCriticalHum + "%<br>";
  ptr += "Lower Critical Humidity: " + (String)_configdata.LowCriticalHum + "%<br>";
  ptr += "Sensing period of critical values: " + (String)_configdata.SensePeriod + " min<br>";
  ptr += "SMS interval when SMS will active: " + (String)_configdata.SMSInterval + " min<br><br>";
  ptr += "SMS will be sent, if temperature and humidity meets any critical value.<br>";
  ptr += "Phone Numbers for SMS sending: ";

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
    Serial.println("configFile" + String(configFile));
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
        ch = jObject["PortalTitle"];
        _configdata.PortalTitle = String(ch);

        ch = jObject["PortalName"];
        _configdata.PortalName = String(ch);
    
    ch = jObject["PlaceName"];
        _configdata.PlaceName = String(ch);
        _configdata.CriticalTemp = jObject["CriticalTemp"];
        _configdata.HiCriticalHum = jObject["HiCriticalHum"];
        _configdata.LowCriticalHum = jObject["LowCriticalHum"];
        _configdata.SensePeriod = jObject["SensePeriod"];
        _configdata.SMSInterval = jObject["SMSInterval"];
        _configdata.PhnNumberCount = jObject["PhnNumberCount"];
        //strcpy(_configdata.PhnNumbers, jObject["PhnNumbers"]);

        Serial.println("Config File data: ");
    
            
        Serial.println("Title of Portal: " + String(_configdata.PortalTitle));
    Serial.println("Name of Portal: " + String(_configdata.PortalName));
        Serial.println("Name of Place: " + String(_configdata.PlaceName));
        Serial.println("Critical temperature: " + String(_configdata.CriticalTemp));
        Serial.println("Lower Critical Humidity: " + String(_configdata.HiCriticalHum));
        Serial.println("Higher Critical Humidity: " + String(_configdata.LowCriticalHum));
        Serial.println("Sensing Period: " + String(_configdata.SensePeriod));
        Serial.println("SMS Interval: " + String(_configdata.SMSInterval));  
    
        Serial.println("Number of phones: " + String(_configdata.PhnNumberCount));
        Serial.println("List of phones: ");


        for(int i=0; i<_configdata.PhnNumberCount; i++)
        {
          ch = jObject["PhnNumbers"][i]; 
          _configdata.PhnNumbers[i] = String(ch);
          Serial.println(_configdata.PhnNumbers[i]);
      
        }      
        delay(1000);
      }
    }
  }
}



void sendSms(String phone, float t, float h)
{

  if(WiFi.status() == WL_CONNECTED) // Check the wifi connection
  {     
    String PayLoad = "Alert!!! In " + _configdata.PlaceName + ", Temperature is " + String(t) + " deg Celcius and humidity is " + String(h) + "%";
    Serial.println(PayLoad);
    PayLoad.replace(" ", "%20");
    Serial.println(PayLoad);
    String link = BaseLink + phone + "&sms=" + PayLoad;
    Serial.println(link);

    http.begin(link);
    int httpCodeT = http.GET();
    String responseBody = http.getString();

    Serial.println(httpCodeT);
    Serial.println(responseBody);

    http.end();
  }
  else 
  {
     Serial.println("Microcontroller is not connected to the wifi device"); 
  }    
  delay(100);      
}

unsigned long timeSinceLastRead = 0;
unsigned int interval = 12000; //12 sec
unsigned long previousMillis = 0;
unsigned long currentMillis;
unsigned long previousMillis4sms = 0;
float temp_store = 0;
float hum_store = 0;
int data_count = 0;
bool send_staus = false;
int issue_count = 0; //sensing period for sms

void loop()
{

  if ( digitalRead(PIN_AP) == LOW ) 
  {
    WiFi.disconnect();
    Serial.println("restart");

    ESP.restart();
    delay(1000);
  }
  
  
  // put your main code here, to run repeatedly:
  server.handleClient();
  ArduinoOTA.handle();  //-------Comment if On The Air is not needed---------
  
  currentMillis = millis();
  timeSinceLastRead = currentMillis - previousMillis;
    if (timeSinceLastRead >= interval) 
    {
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float h = dht.readHumidity();
      //Serial.println(h);
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      //Serial.println(t);
      // Read temperature as Fahrenheit (isFahrenheit = true)
      //float f = dht.readTemperature(true);
      
  
      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t)) 
      {
        Serial.println("Failed to read from DHT sensor!");
        timeSinceLastRead = 0;
        previousMillis = currentMillis;
        return;
      }
      temp_store = temp_store + t;
      hum_store = hum_store + h;
      data_count++;
      if (data_count == 5) 
      {
        t = temp_store / 5.0;
        h = hum_store / 5.0;
        temp_store = 0;
        hum_store = 0;
        data_count = 0;
        Serial.println(t);
        Serial.println(h);
        
        String web_link = CloudLink + "&field1=" + String(t) + "&field2=" + String(h);
        Serial.println("Web Url: " + web_link);
        http.begin(web_link);
        int httpCodeT = http.GET();
        String responseBody = http.getString();
        Serial.println("Sending temperature data to server. And response code is: " + String(httpCodeT) + " & response: " + responseBody);
        http.end();
        
        if(_configdata.PhnNumberCount > 0 && _configdata.CriticalTemp != 0 && _configdata.HiCriticalHum > 0 && _configdata.SMSInterval > 0 && _configdata.SensePeriod > 0)
        {
          Serial.println("config data present");
          if(t > _configdata.CriticalTemp || h > _configdata.HiCriticalHum || h < _configdata.LowCriticalHum)    
          {
            if(!SMSStopped)
            {
              Serial.println("condition achieved");
              if(!send_staus)
              {
                issue_count++;
                Serial.println("issue count: " + String(issue_count));
                if(issue_count == _configdata.SensePeriod)//5/////
                {
                  SMSRunning = true;                
                  for(int i=0; i<_configdata.PhnNumberCount; i++)
                  {
                    Serial.println("sending sms to: " + _configdata.PhnNumbers[i]);
                    sendSms(_configdata.PhnNumbers[i], t, h);                                                           
                  }
                  issue_count = 0;
                  send_staus = true;              
                }
              }
              if(currentMillis - previousMillis4sms >= _configdata.SMSInterval*60000)/////
              {            
                send_staus = false;
                previousMillis4sms = currentMillis;
                Serial.println("send status true");
  
              }              
            }            
          }
          else
          {
            SMSStopped = false;
            issue_count = 0;
            send_staus = false;
            previousMillis4sms = 0;
            SMSRunning = false;
            //Serial.println("condition not achieved");
          }
        }
      }
  
    timeSinceLastRead = 0;
    previousMillis = currentMillis;
  }
}
