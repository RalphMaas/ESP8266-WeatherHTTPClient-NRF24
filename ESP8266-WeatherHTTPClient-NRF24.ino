#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//needed for library
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/kentaylor/WiFiManager

// Constants
const int PIN_LED = 2; // D4 on NodeMCU and WeMos. Controls the onboard LED.
const int PIN_LED_CONFIG = 4; // D2 In config mode.
const int TRIGGER_PIN = 0; // D3 on NodeMCU and WeMos, set MCU con configmode.
const char* CONFIG_FILE = "/config.json";

// Variables
// Indicates whether ESP has WiFi credentials saved from previous session
bool initialConfig = false;

// Default configuration values
char knmiApiKey[17] = "";
char knmiApiKeyCity[20] = "";

// Function Prototypes

bool readConfigFile();
bool writeConfigFile();

// Setup function
void setup() {
  // Put your setup code here, to run once
  Serial.begin(115200);
  Serial.println("\n Starting");

  // Initialize the LED digital pin as an output.
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_LED_CONFIG, OUTPUT);
  // Initialize trigger pins
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  
  // Mount the filesystem
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + result);

  if (!readConfigFile()) {
    Serial.println("Failed to read configuration file, using default values");
  }

  WiFi.printDiag(Serial); //Remove this line if you do not want to see WiFi password printed

  if (WiFi.SSID() == "") {
    Serial.println("We haven't got any access point credentials, so get them now");
    initialConfig = true;
  } else {
    digitalWrite(PIN_LED, HIGH); // Turn LED on as we are  in configuration mode.
    digitalWrite(PIN_LED_CONFIG, LOW); // Turn LED off as we are in configuration mode.
    WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
    unsigned long startedAt = millis();
    Serial.print("After waiting ");
    int connRes = WiFi.waitForConnectResult();
    float waited = (millis()- startedAt);
    Serial.print(waited/1000);
    Serial.print(" secs in setup() connection result is ");
    Serial.println(connRes);
  }

  if (WiFi.status()!=WL_CONNECTED){
    Serial.println("Failed to connect, finishing setup anyway");
  } else{
    Serial.print("Local ip: ");
    Serial.println(WiFi.localIP());
  }
}

// Loop function

void loop() {
  // is configuration portal requested?
  if ((digitalRead(TRIGGER_PIN) == LOW) || (initialConfig)) {
     Serial.println("Configuration portal requested");
     digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
    digitalWrite(PIN_LED_CONFIG, HIGH); // In cofigmode.
    
    //Local intialization. Once its business is done, there is no need to keep it around

    // Extra parameters to be configured
    // After connecting, parameter.getValue() will get you the configured value
    // Format: <ID> <Placeholder text> <default value> <length> <custom HTML> <label placement>

    // KNMI API Key - this is a straight forward string parameter
    WiFiManagerParameter p_knmiApiKey("knmiApiKey", "KNMI API Key", knmiApiKey, 17);
    WiFiManagerParameter p_knmiApiKeyCity("knmiApiKeyCity", "City", knmiApiKeyCity, 20);
    
    // Just a quick hint
    WiFiManagerParameter p_hint("<small>*Hint: if you want to reuse the currently active WiFi credentials, leave SSID and Password fields empty</small>");
    
    // Initialize WiFIManager
    WiFiManager wifiManager;
    
    //add all parameters here
    
    wifiManager.addParameter(&p_hint);
    wifiManager.addParameter(&p_knmiApiKey);
    wifiManager.addParameter(&p_knmiApiKeyCity);

    

    // Sets timeout in seconds until configuration portal gets turned off.
    // If not specified device will remain in configuration mode until
    // switched off via webserver or device is restarted.
    // wifiManager.setConfigPortalTimeout(600);

    // It starts an access point 
    // and goes into a blocking loop awaiting configuration.
    // Once the user leaves the portal with the exit button
    // processing will continue
    if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      // If you get here you have connected to the WiFi
      Serial.println("Connected...yeey :)");
    }


    // Getting posted form values and overriding local variables parameters
    // Config file is written regardless the connection state
    strcpy(knmiApiKey, p_knmiApiKey.getValue());
    strcpy(knmiApiKeyCity, p_knmiApiKeyCity.getValue());

    
    // Writing JSON config file to flash for next boot
    writeConfigFile();

    
    digitalWrite(PIN_LED, HIGH); // Turn LED off as we are not in configuration mode.

    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up 
    // so resetting the device allows to go back into config mode again when it reboots.
    delay(5000);
  }

  // Configuration portal not requested, so run normal loop
  // Put your main code here, to run repeatedly...

}

bool readConfigFile() {
  // this opens the config file in read-mode
  File f = SPIFFS.open(CONFIG_FILE, "r");
  
  if (!f) {
    Serial.println("Configuration file not found");
    return false;
  } else {
    // we could open the file
    size_t size = f.size();
    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    // Read and store file contents in buf
    f.readBytes(buf.get(), size);
    // Closing file
    f.close();
    // Using dynamic JSON buffer which is not the recommended memory model, but anyway
    // See https://github.com/bblanchon/ArduinoJson/wiki/Memory%20model
    DynamicJsonBuffer jsonBuffer;
    // Parse JSON string
    JsonObject& json = jsonBuffer.parseObject(buf.get());
    // Test if parsing succeeds.
    if (!json.success()) {
      Serial.println("JSON parseObject() failed");
      return false;
    }
    json.printTo(Serial);

    // Parse all config file parameters, override 
    // local config variables with parsed values
    if (json.containsKey("knmiApiKey")) {
      strcpy(knmiApiKey, json["knmiApiKey"]);      
    }
    
    if (json.containsKey("knmiApiKeyCity")) {
      strcpy(knmiApiKeyCity, json["knmiApiKeyCity"]);      
    }
  }
  Serial.println("\nConfig file was successfully parsed");
  return true;
}

bool writeConfigFile() {
  Serial.println("Saving config file");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  // JSONify local configuration parameters
  json["knmiApiKey"] = knmiApiKey;
  json["knmiApiKeyCity"] = knmiApiKeyCity;

  // Open file for writing
  File f = SPIFFS.open(CONFIG_FILE, "w");
  if (!f) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.prettyPrintTo(Serial);
  // Write data to file and close it
  json.printTo(f);
  f.close();

  Serial.println("\nConfig file was successfully saved");
  return true;
}
  
