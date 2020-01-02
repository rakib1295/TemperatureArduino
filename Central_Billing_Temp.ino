#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
include <ESPAsyncWebServer.h>
//#include <ESP8266WebServer.h>
#include <WiFiManager.h>  
include <ESPAsyncTCP.h>
//https://github.com/tzapu/WiFiManager

//for LED status
#include <Ticker.h>
Ticker ticker;


char webpage[] PROGMEM = R"rawliteral(
<html>
<head>
<title>Config Form for Sensor Device</title>
</head>
<body>
<h1 align="middle">Config SMS Settings</h1>
  <form align="middle">
    <fieldset>
      <div>
        <label for="CriticalTemp">Critical Temperature: </label>      
        <input id="CriticalTemp" type="number" placeholder="Critical Temperature">°C</input>
      </div><br>
      <div>
        <label for="HiCriticalHum">Higher Critical Humidity: </label>
        <input id="HiCriticalHum" type="number" placeholder="Higher Critical Humidity">%</input>
      </div><br>
    <div>
        <label for="LowCriticalHum">Lower Critical Humidity: </label>
        <input id="LowCriticalHum" type="number" placeholder="Lower Critical Humidity">%</input>
      </div><br>    
    
    <div>
        <label for="PhnNumberCount">Phone Number count: </label>
        <input type="number" id="PhnNumberCount" placeholder="Give total count to add">   
    <button id="AddPhnNumbers" type="button" onclick="addFields()">Create forms</button>
    <br><br>
    <div id="container"/>
    </div><br> 
    <br>
      <div>
        <button class="primary" id="savebtn" type="button" onclick="myFunction()">SAVE</button>
      </div>
    </fieldset>
  </form>
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
      input.type = "text";
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
    var CriticalTemp = document.getElementById("CriticalTemp").value;
    var HiCriticalHum = document.getElementById("HiCriticalHum").value;
    var LowCriticalHum = document.getElementById("LowCriticalHum").value;
    var PhnNumbers = document.getElementById("container").value;
    
    var number = document.getElementById("PhnNumberCount").value;
    var PhnNumbers = document.getElementById("container");
    var phndata = [];
    for (i=0;i<number;i++)
    {
      if(document.getElementById("PhnNumber" + i).value != "") //if blank form, then will ignore
      {
        phndata[i] = document.getElementById("PhnNumber" + i).value;
      }
    }
      
    
    var data = {CriticalTemp:CriticalTemp, HiCriticalHum:HiCriticalHum, LowCriticalHum:LowCriticalHum, PhnNumbers:phndata};
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
        console.log(xhr.responseText);
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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
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
  wifiManager.setSTAStaticIPConfig(IPAddress(192, 168, 0, 191), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));

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
}

void loop() {
  // put your main code here, to run repeatedly:

}
