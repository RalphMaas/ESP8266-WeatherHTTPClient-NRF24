#include <SPI.h> //Call SPI library so you can communicate with the nRF24L01+
#include <nRF24L01.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <RF24.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <LiquidCrystal_I2C.h>

//LCD 1602
const int  en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3;
const int i2c_addr = 0x27;
//LiquidCrystal_I2C lcd(i2c_addr, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE);

const unsigned long interval = 4500;// 8s;
unsigned long last_clear;

const uint8_t pinCE = 9; //This pin is used to set the nRF24 to standby (0) or active mode (1)
const uint8_t pinCSN = 10; //This pin is used for SPI comm chip select
RF24 myRadio(pinCE, pinCSN); // Declare object from nRF24 library (Create your wireless SPI) 

const uint64_t rAddress = 0xB00B1E50C3LL;  //Create pipe address to send data, the "LL" is for LongLong type
const uint8_t rFChan = 89; //Set channel default (chan 84 is 2.484GHz to 2.489GHz)
const uint8_t rDelay = 7; //this is based on 250us increments, 0 is 250us so 7 is 2 ms
const uint8_t rNum = 5; //number of retries that will be attempted 

const unsigned long pingInterval = 1000;// ms;
unsigned long last_sent;  

typedef struct {
  String  Sup;
  String  Sunder;
} SupSunder;

SupSunder supsunder;


void setupNRF24(){
  Serial.println("Start Setup NRF24");
  myRadio.begin();  //Start the nRF24 module
  myRadio.setChannel(rFChan); //set communication frequency channel
  myRadio.openReadingPipe(1,rAddress);  //This is receiver or master so we need to be ready to read data from transmitters
  myRadio.stopListening();    // Start listening for messages
  Serial.println("End Setup NRF24");
}/*
void InitDisplay()
{
    unsigned long now = millis();
   if (now - last_clear >= interval ) {
      last_clear = now;
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("STAND BY...");
   }
}
*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  //serial port to display received data
  Serial.println("Setup");
/*  lcd.begin(16,2);
  lcd.clear();
  lcd.home();
  lcd.setCursor(2,0);
  lcd.print("INIT...");
  */
  setupNRF24();
}

void loop() {
   unsigned long now = millis();
   
    if (now - last_sent >= pingInterval ) { 
          Serial.println("try to send...");
          last_sent = now;
          
          supsunder.Sup = "08:30";
          supsunder.Sunder = "19:37";
             
         if (!myRadio.write(&supsunder, sizeof(supsunder))){  //send data and remember it will retry if it fails
            Serial.println("payload NOT SEND");
    
            delay(random(5,20)); //as another back up, delay for a random amount of time and try again
            if (!myRadio.write(&supsunder, sizeof(supsunder))){
              Serial.println("payload NOT SEND");
            }
            else
            {
              Serial.println("payload SEND");
            }
         }
         else
         {
              Serial.println("payload SEND");
         }
    }  
}
