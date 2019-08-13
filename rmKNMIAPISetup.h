// Constants
const int PIN_LED_GREEN = 4; // GPIO 4 LED GREEN .
const int PIN_LED_CONFIG_BLUE = 5; // GPIO 5 In LED BLUE (config mode).
const int PIN_SWITCH_CONFIG = 16; // GPIO 16 set MCU configmode.
const int PIN_SWITCH_API_ON = 0; //GPIO 0 set the api read on. Off is only for test.

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

