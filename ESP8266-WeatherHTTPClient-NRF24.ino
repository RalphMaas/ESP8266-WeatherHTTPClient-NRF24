
//#include <FS.h>
//#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
//#include <ESP8266HTTPClient.h>

//needed for library
//#include <ESP8266WebServer.h>
//#include <DNSServer.h>
#include "rmCommon.h"

#include <SPI.h> //Call SPI library so you can communicate with the nRF24L01+
#include <nRF24L01.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <RF24.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
//#include <WiFiManager.h>          //https://github.com/kentaylor/WiFiManager

//NRF24

const uint8_t PIN_NRF24_CE = 2;//7; //This pin is used to set the nRF24 to standby (0) or active mode (1)
const uint8_t PIN_NRF24_CSN =14;// 8; //This pin is used to tell the nRF24 whether the SPI communication is a command
//RF24 wirelessSPI(PIN_NRF24_CE, PIN_NRF24_CSN); // Declare object from nRF24 library (Create your wireless SPI) 

const uint64_t wAddress = 0xB00B1E50C3LL;  //Create pipe address to send data, the "LL" is for LongLong type
const uint8_t rFChan = 89; //Set channel default (chan 84 is 2.484GHz to 2.489GHz)
const uint8_t rDelay = 7; //this is based on 250us increments, 0 is 250us so 7 is 2 ms
const uint8_t rNum = 5; //number of retries that will be attempted 

RF24 myRadio(PIN_NRF24_CE,PIN_NRF24_CSN);

// Constants
const int PIN_LED_GREEN = 4; // GPIO 4 LED GREEN .
const int PIN_LED_CONFIG_BLUE = 5; // GPIO 5 In LED BLUE (config mode).
const int PIN_SWITCH_CONFIG = 16; // GPIO 16 set MCU configmode.
const int PIN_SWITCH_API_ON = 0; //GPIO 0 set the api read on. Off is only for test.
const char* CONFIG_FILE = "/config.json";

// Variables
bool  READ_API = false;
bool initialConfig = false;
SupSunder supsunder;

Weather weather_base_info;
ForeCast weather_today;
ForeCast weather_tomorrow;
ForeCast weather_DayAfterTomorrow;
WeatherText weather_text;
WeatherText weather_warn;

unsigned long previousMillis = 0;
unsigned long interval = 10000;

// Default configuration values
char knmiApiKey[17] = "";
char knmiApiKeyCity[20] = "";

/*
void checkConfigurationRequested(){
  // is configuration portal requested?
  if ((digitalRead(PIN_SWITCH_CONFIG) == LOW) || (initialConfig)) {
     Serial.println("Configuration portal requested");
 
     digitalWrite(PIN_LED_GREEN, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
     digitalWrite(PIN_LED_CONFIG_BLUE, HIGH); // In configmode.
    
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

    
   // digitalWrite(PIN_LED_GREEN, HIGH); // Turn LED off as we are not in configuration mode.

    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up 
    // so resetting the device allows to go back into config mode again when it reboots.
    delay(5000);
  }
}

void SetupLedAndSwitches(){
  // Initialize the LED digital pin as an output.
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_CONFIG_BLUE, OUTPUT);
  // Initialize switches
  pinMode(PIN_SWITCH_CONFIG, INPUT_PULLUP);
  pinMode(PIN_SWITCH_API_ON, INPUT_PULLUP);
  // default LED Off 
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_CONFIG_BLUE, LOW);
}

void TestLedAndSwitches(){
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_CONFIG_BLUE, HIGH);
  if (digitalRead(PIN_SWITCH_CONFIG) == LOW)
    Serial.println("config pushed");
  if (digitalRead(PIN_SWITCH_API_ON) == LOW)
    Serial.println("API pushed");  
}

void ReadKNMIApi(HTTPClient& http){
    readConfigFile();
    Serial.print("API Key: ");
    Serial.println(knmiApiKey);
    
    Serial.print("API Location: ");
    Serial.println(knmiApiKeyCity);
    String url = (String)"http://weerlive.nl/api/json-data-10min.php?key="+knmiApiKey+"&locatie="+knmiApiKeyCity+"";
    Serial.print("Url: ");
    Serial.println(url);

   
    if (READ_API)
    {
        Serial.print("API Live");
        //example api call : http://weerlive.nl/api/json-data-10min.php?key=051d293ebf&locatie=Middelburg
        http.begin("http://weerlive.nl/api/json-data-10min.php?key=051d293ebf&locatie=Middelburg");
        int httpCode = http.GET();
    
        if (httpCode > 0) {
          String result = http.getString();
          
          Serial.println(result);
          const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(49) + 670;
          DynamicJsonBuffer jsonBuffer(bufferSize);
          JsonObject& root = jsonBuffer.parseObject(result);
          JsonObject& liveweer0 = root["liveweer"][0];
          
          //Base data
          const char* liveweer0_plaats = liveweer0["plaats"]; // "Middelburg"
          const char* liveweer0_temp = liveweer0["temp"]; // "8.3"
          const char* liveweer0_luchtd = liveweer0["luchtd"]; // "08:45"
          const char* liveweer0_sup = liveweer0["sup"]; // "08:45"
          const char* liveweer0_sunder = liveweer0["sunder"]; // "16:40"
    
          weather_base_info.PackId = CONST_WTHDFT; 
          weather_base_info.City = liveweer0_plaats;
          weather_base_info.Temp = liveweer0_temp;
          weather_base_info.AirPress = liveweer0_luchtd;
          weather_base_info.Sup = liveweer0_sup;
          weather_base_info.Sunder = liveweer0_sunder;
           

         //Today
          const char* liveweer0_icon0 = liveweer0["d0weer"];
          const char* liveweer0_maxtemp0 = liveweer0["d0tmax"];
          const char* liveweer0_mintemp0 = liveweer0["d0tmin"];
          const char* liveweer0_wind0 = liveweer0["d0windk"];
          const char* liveweer0_winddir0 = liveweer0["d0windr"];
          const char* liveweer0_rainprocent0 = liveweer0["d0neerslag"];
          const char* liveweer0_sunprocent0 = liveweer0["d0zon"];

          weather_today.PackId = CONST_WTHTOD;
          weather_today.Icon = liveweer0_icon0;
          weather_today.MaxTemp = liveweer0_maxtemp0;
          weather_today.MinTemp = liveweer0_mintemp0;
          weather_today.Wind = liveweer0_wind0;
          weather_today.WindDir = liveweer0_winddir0;
          weather_today.RainProcent = liveweer0_rainprocent0;
          weather_today.SunProcent = liveweer0_sunprocent0;

       
          //Tomorrow
          const char* liveweer0_icon1 = liveweer0["d1weer"];
          const char* liveweer0_maxtemp1 = liveweer0["d1tmax"];
          const char* liveweer0_mintemp1 = liveweer0["d1tmin"];
          const char* liveweer0_wind1 = liveweer0["d1windk"];
          const char* liveweer0_winddir1 = liveweer0["d1windr"];
          const char* liveweer0_rainprocent1 = liveweer0["d1neerslag"];
          const char* liveweer0_sunprocent1 = liveweer0["d1zon"];

          weather_tomorrow.PackId = CONST_WTHTMO;
          weather_tomorrow.Icon = liveweer0_icon1;
          weather_tomorrow.MaxTemp = liveweer0_maxtemp1;
          weather_tomorrow.MinTemp = liveweer0_mintemp1;
          weather_tomorrow.Wind = liveweer0_wind1;
          weather_tomorrow.WindDir = liveweer0_winddir1;
          weather_tomorrow.RainProcent = liveweer0_rainprocent1;
          weather_tomorrow.SunProcent = liveweer0_sunprocent1;
          

          // Day after tomorrow
          const char* liveweer0_icon2 = liveweer0["d2weer"];
          const char* liveweer0_maxtemp2 = liveweer0["d2tmax"];
          const char* liveweer0_mintemp2 = liveweer0["d2tmin"];
          const char* liveweer0_wind2 = liveweer0["d2windk"];
          const char* liveweer0_winddir2 = liveweer0["d2windr"];
          const char* liveweer0_rainprocent2 = liveweer0["d2neerslag"];
          const char* liveweer0_sunprocent2 = liveweer0["d2zon"];

          weather_DayAfterTomorrow.PackId = CONST_WTHDAT;
          weather_DayAfterTomorrow.Icon = liveweer0_icon2;
          weather_DayAfterTomorrow.MaxTemp = liveweer0_maxtemp2;
          weather_DayAfterTomorrow.MinTemp = liveweer0_mintemp2;
          weather_DayAfterTomorrow.Wind = liveweer0_wind2;
          weather_DayAfterTomorrow.WindDir = liveweer0_winddir2;
          weather_DayAfterTomorrow.RainProcent = liveweer0_rainprocent2;
          weather_DayAfterTomorrow.SunProcent = liveweer0_sunprocent2;

          // weather text
          const char* liveweer0_text = liveweer0["verw"];

          weather_text.PackId = CONST_WTHTXT;
          weather_text.Text = liveweer0_text;
          
       }
        http.end(); //Close connection
      }
      else {
          Serial.print("API Test");
          weather_base_info.PackId = "BASE"; 
          weather_base_info.City = "City";
          weather_base_info.Temp = "Temp";
          weather_base_info.AirPress = "AirPress";
          weather_base_info.Sup = "Sup";
          weather_base_info.Sunder = "Sunder";

           weather_today.PackId = "TODAY";
          weather_today.Icon = "Icon";
          weather_today.MaxTemp = "Max";
          weather_today.MinTemp = "Min";
          weather_today.Wind = "Wind";
          weather_today.WindDir = "Dir";
          weather_today.RainProcent = "Rain";
          weather_today.SunProcent = "Sun";
      }
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

void setupConfig(){
  // Mount the filesystem
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + result);

  if (!readConfigFile()) {
    Serial.println("Failed to read configuration file, using default values");
  }
}

void setupWifi(){
  WiFi.printDiag(Serial); //Remove this line if you do not want to see WiFi password printed

  if (WiFi.SSID() == "") {
    Serial.println("We haven't got any access point credentials, so get them now");
    initialConfig = true;
  } else {
    digitalWrite(PIN_LED_GREEN, HIGH); // Turn LED on as we are  in configuration mode.
    digitalWrite(PIN_LED_CONFIG_BLUE, LOW); // Turn LED off as we are in configuration mode.
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
*/

//const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };   // Radio pipe addresses for the 2 nodes to communicate.

void setupNRF24(){
  Serial.println("Start Setup NRF24");
  myRadio.begin();  //Start the nRF24 module
  myRadio.setChannel(rFChan); 
  myRadio.setRetries(rDelay,rNum); //if a transmit fails to reach receiver (no ack packet) then this sets retry attempts and delay between retries   
  myRadio.openWritingPipe(wAddress); //open writing or transmit pipe
  myRadio.stopListening(); //go into transmit mode


 /* myRadio.begin();                           // Setup and configure rf radio
  myRadio.setChannel(1);
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate(RF24_1MBPS);
  myRadio.setAutoAck(1);                     // Ensure autoACK is enabled
  myRadio.setRetries(2,15);                  // Optionally, increase the delay between retries & # of retries
  
  myRadio.setCRCLength(RF24_CRC_8);          // Use 8-bit CRC for performance
  myRadio.openWritingPipe(pipes[0]);
  myRadio.powerUp(); 
  */
  Serial.println("End Setup NRF24");

}

void setup() {
  // Put your setup code here, to run once
  Serial.begin(115200);
  Serial.println("\n Starting");
 // SetupLedAndSwitches();
  setupNRF24();
//  writeConfigFile();
//  setupWifi();
}
/*
void dumpData(){
   Serial.println("Base Data");
  Serial.println(weather_base_info.PackId);
  Serial.println(weather_base_info.City);
  Serial.println(weather_base_info.Temp);
  Serial.println(weather_base_info.AirPress);
  Serial.println(weather_base_info.Sup);
  Serial.println(weather_base_info.Sunder);

  if (READ_API )
  {

      Serial.println(weather_today.PackId);
      Serial.println(weather_today.Icon);
      Serial.println(weather_today.MaxTemp);
      Serial.println(weather_today.MinTemp);
      Serial.println(weather_today.Wind);
      Serial.println(weather_today.WindDir);
      Serial.println(weather_today.RainProcent);
      Serial.println(weather_today.SunProcent);

      Serial.println(weather_tomorrow.PackId);
      Serial.println(weather_tomorrow.Icon);
      Serial.println(weather_tomorrow.MaxTemp);
      Serial.println(weather_tomorrow.MinTemp);
      Serial.println(weather_tomorrow.Wind);
      Serial.println(weather_tomorrow.WindDir);
      Serial.println(weather_tomorrow.RainProcent);
      Serial.println(weather_tomorrow.SunProcent);

      Serial.println(weather_DayAfterTomorrow.PackId);
      Serial.println(weather_DayAfterTomorrow.Icon);
      Serial.println(weather_DayAfterTomorrow.MaxTemp);
      Serial.println(weather_DayAfterTomorrow.MinTemp);
      Serial.println(weather_DayAfterTomorrow.Wind);
      Serial.println(weather_DayAfterTomorrow.WindDir);
      Serial.println(weather_DayAfterTomorrow.RainProcent);
      Serial.println(weather_DayAfterTomorrow.SunProcent);

      Serial.println(weather_text.PackId);
      Serial.println(weather_text.Text);
    }
}
*/
void loop() {
/*  checkConfigurationRequested();
  if (WiFi.status() == WL_CONNECTED) 
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      READ_API = digitalRead(PIN_SWITCH_API_ON) == LOW;
      previousMillis = currentMillis;
      HTTPClient http; //Object of class HTTPClient
 
      ReadKNMIApi(http);
      dumpData();
    }
  }
 */   
      
    Serial.println("start NRF24");
 
 
    supsunder.Sup = "07:30";
    supsunder.Sunder = "21:34";
    

    Serial.print("sizeof message : ");
    Serial.println(sizeof(supsunder));
   /* if(!myRadio.writeFast(&supsunder,sizeof(supsunder))){           
        Serial.println("NRF24 - Error sending message");
     }
     else {
        Serial.println("NRF24 send message done.");
     }
     */     
   //Send default info
   if (!myRadio.write(&supsunder, sizeof(supsunder))){  
      delay(random(5,20)); 
      if (!myRadio.write(&supsunder, sizeof(supsunder))){
         Serial.println("NRF24 - Error sending message");
      }
   }
   
   delay(1000);    
     //Send default today data
     //Send default tomorrow data
     //Send default day after today data
     //Send default day text data
}
