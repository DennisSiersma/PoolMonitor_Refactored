// This code uses wifi manageer to autoselect a pre-saved Access point or use captive portal 
// it also allows you to swtich between local and cloud server's

// todo:
// - try auto switchover based on connection attempts
// 

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <ESP8266WebServer.h>
//#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
//#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
//#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <TFT_ILI9341_ESP.h>                    // Hardware-specific library
#include <Wire.h>                               // enable I2C.
#include <OneWire.h>                            // enable OneWire
#include <DallasTemperature.h>                  // Helper for watertemp sensor
#include <Ticker.h>                             // Timer for watchdog 
#include <TimeLib.h>

// Additional UI functions
//#include "GfxUi.h"


#define vPIN_localORcloud_server 127

// Pins for Feather Huzzah
// I2C SDA = GPIO #4 (default), I2C SCL = GPIO #5 (default)
// SPI SCK = GPIO #14 (default), SPI MOSI = GPIO #13 (default, SPI MISO = GPIO #12 (default)
// Pins for the TFT interface are defined in the User_Config.h file inside the TFT_ILI9341_ESP library
// These are the ones I used on a Feather Huzzah:
//#define TFT_CS   8  // Chip select control pin D8
//#define TFT_DC   3  // Data Command control pin
//#define TFT_RST  4  // Reset pin (could connect to NodeMCU RST, see next line)

//  #define D0  16
//  #define D1  5
//  #define D2  4
//  #define D3  0
#define D4  2
//  #define D5  14
//  #define D6  12
//  #define D7  13
//  #define D8  15
//  #define RX  3
//  #define TX  1

#define ONE_WIRE_BUS 2              //data pin for tempsensor, use a 4.7k Ohm resistor between data and vcc!
#define TEMPERATURE_PRECISION 9     //9 bit precision

/***********************************************************************************************
 * Credentials                                                                                 *     
 ***********************************************************************************************/
char auth_cloud_server[] = "0a3b9f9e4c3446508446b726ffb6a66f"; // blynk public cloud server

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Siersma2";
char pass[] = "";
const char* host = "POOLMON";
/***********************************************************************************************
 * Init sensors                                                                                *     
 ***********************************************************************************************/

BlynkTimer timer;
WidgetTerminal terminal(V10);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
TFT_ILI9341_ESP tft = TFT_ILI9341_ESP();       // Invoke custom library

String inputString = "";

String currentTime = String(hour()) + ":" + minute() + ":" + second();
String currentDate = String(day()) + " " + month() + " " + year();

// flags used to switch between cloud and local Blynk server
//bool cloud_server_active = true;
//bool local_server_active = false;
int n = 0; //counter for heartbeat

/***********************************************************************************************
 * Setup                                                                                       *     
 ***********************************************************************************************/

 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("\n Starting");
  Wire.begin();                                                               // enable I2C port.
  sensors.begin();                                                            // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  //next_serial_time = millis() + send_readings_every;                          // calculate the next point in time we should do serial communications

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  //tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextFont(2);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Gebouwd door DJS 2017", 120, 240);
  tft.drawString("Groeten Thom & Chris", 120, 260);
  delay(500);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  int waittime = millis() + 10000;
  bool x = true;
  
  WiFiManager wifiManager;
      
 if (!wifiManager.autoConnect("AutoConnectAP")) {     // might not need if statement
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000); }
 /*   
 WiFi.begin(ssid, pass);          // use this code if we want to hardwire an access point
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (waittime < millis()) { break; }
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  
  */
  // 
 
  Blynk.config(auth_cloud_server);
  Blynk.connect(3333);  // timeout set to 10 seconds and then continue without Blynk - thank Costas :-)
  while(Blynk.connect() == false);
 

  Blynk.virtualWrite(V10, "\n\n");
  Blynk.virtualWrite(V10, "    ___  __          __ \n");
  Blynk.virtualWrite(V10, "   / _ )/ /_ _____  / /__ \n");
  Blynk.virtualWrite(V10, "  / _  / / // / _ \\/  '_/ \n");  // to display a backslash, print it twice
  Blynk.virtualWrite(V10, " /____/_/\\_, /_//_/_/\\_\\ \n");  // to display a backslash, print it twice
  Blynk.virtualWrite(V10, "        /___/ v");
  Blynk.virtualWrite(V10, BLYNK_VERSION" on "BLYNK_INFO_DEVICE"\n\n");
  Blynk.virtualWrite(V10, "                 Project By DJS -\n");
  Blynk.virtualWrite(V10, "                 ... Starting!\n");
  terminal.flush();
  
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() { // 
                        terminal.println("OTA starting....");
                    });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
                          terminal.println("Done.");
                        });

   ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });

   /* setup the OTA server */
   ArduinoOTA.begin();
   terminal.println("OTA Ready");
   
   
/***********************************************************************************************
 * Setup timers                                                                                *     
 ***********************************************************************************************/
  timer.setInterval (5000L, heartbeat);  // Draw time
  timer.setInterval (500L, Sent_serial); // check to see if anything to send from hardware serial to Blynk terminal
  timer.setInterval(10000L, requestTime); // sync time with Blynk rtc
  timer.setInterval(15000L, reconnectBlynk); // check to see if we are still connected to Blynk
  timer.setInterval (10000L, updateData); // update sensors
}

/***********************************************************************************************
 * Blynk rtc function                                                          *     
 ***********************************************************************************************/
void requestTime() {
  Blynk.sendInternal("rtc", "sync");
  
  currentTime = String(hour()) + ":" + minute() + ":" + second();
  currentDate = String(day()) + " " + month() + " " + year();
  //terminal.println(currentTime);
}
/***********************************************************************************************
 * Serial interupt    (debugging)                                                              *     
 ***********************************************************************************************/
void Sent_serial() {
       // Sent serial data to Blynk terminal - Unlimited string read
       String content = "";  //null string constant ( an empty string )
       char character;
       while(Serial.available()) {
            character = Serial.read();
            content.concat(character);
          //  Serial.println ("in serial loop");  
       }
       if (content != "") {
            Blynk.virtualWrite (V10, content);  // send serial to blynk terminal
       } 
}

/***********************************************************************************************
 * Heartbeat    (debugging)                                                              *     
 ***********************************************************************************************/
void heartbeat()
{
  if (Blynk.connected()){
  Serial.print("Blynk Connected & still going....");
  Serial.println (n);
  n++;
  Blynk.virtualWrite(V0, n);  // Set a valuewidget to see heartbeat
  drawTime();
  }
}

/***********************************************************************************************
 * EZO stuff                                                                                   *     
 ***********************************************************************************************/
#define TOTAL_CIRCUITS 2                            // <-- CHANGE THIS |Â set how many I2C circuits are attached to the Tentacle

//const unsigned int baud_host  = 9600;               // set baud rate for host serial monitor(pc/mac/other)
const unsigned int send_readings_every = 50000;     // set at what intervals the readings are sent to the computer (NOTE: this is not the frequency of taking the readings!)
unsigned long next_serial_time;
const int UPDATE_TEMP_SECS = 30; 
char sensordata[30];                                // A 30 byte character array to hold incoming data from the sensors
byte sensor_bytes_received = 0;                     // We need to know how many characters bytes have been received

byte code = 0;                                      // used to hold the I2C response code.
byte in_char = 0;                                   // used as a 1 byte buffer to store in bound bytes from the I2C Circuit.

int channel_ids[] = {98, 99};                       // <-- CHANGE THIS. A list of I2C ids that you set your circuits to.
char *channel_names[] = {"ORP", "PH"};              // <-- CHANGE THIS. A list of channel names (must be the same order as in channel_ids[]) - only used to designate the readings in serial communications

String readings[TOTAL_CIRCUITS];                    // an array of strings to hold the readings of each channel
String TEMP_val = "Hold";
String PH_val = "one";
String ORP_val = "sec";

char command_string[20];       // holds command to be send to probe

int cmdCalORP = 0;
int cmdCalPH7 = 0;
int cmdCalPH4 = 0;
int cmdCalPH10 = 0;
int cmdCalCORP = 0;
int cmdCalCPH = 0;
int cmdTempComp = 0;

char ScmdCalORP[] = "Cal,225";
char ScmdCalPH7[] = "Cal,mid,7.00";
char ScmdCalPH4[] = "Cal,low,4.00";
char ScmdCalPH10[] = "Cal,high,10.00";
char ScmdCalCORP[] = "Cal,clear";
char ScmdCalCPH[] = "Cal,clear";  
char ScmdTempComp[] = "T,";  
byte cs_lenght;                               // counter for char lenght

int channel = 0;                              // INT pointer to hold the current position in the channel_ids/channel_names array

const unsigned int reading_delay = 1400;      // time to wait for the circuit to process a read command. datasheets say 1 second.
unsigned long next_reading_time;              // holds the time when the next reading should be ready from the circuit
boolean request_pending = false;              // wether or not we're waiting for a reading

const unsigned int blink_frequency = 250;     // the frequency of the led blinking, in milliseconds
unsigned long next_blink_time;                // holds the next time the led should change state
boolean led_state = LOW;                      // keeps track of the current led state


void drawSeparator(uint16_t y) {
  tft.drawFastHLine(10, y, 240 - 2 * 10, 0x4228);
}
/***********************************************************************************************
 * updateData                                                                                       *     
 ***********************************************************************************************/
void updateData() {
  do_sensor_readings();
  do_serial();
  requestTemp();
}

/***********************************************************************************************
 * drawData                                                                                       *     
 ***********************************************************************************************/
void drawData() {
  tft.fillScreen(TFT_BLACK);
  drawEZO();
  }
/***********************************************************************************************
 * update Screen                                                                                       *     
 ***********************************************************************************************/
void drawTime() {
  //tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextFont(2);
  
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Ddd, 44 Mmm 4444 "));  // String width + margin
  tft.drawString(currentDate, 120, 14);

  //tft.setFreeFont(&ArialRoundedMTBold_36);
  tft.setTextFont(6);
  
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 44:44 "));  // String width + margin
  tft.drawString(currentTime, 120, 50);

  drawSeparator(52);
  drawSeparator(153);
  tft.setTextPadding(0);
}
 
void drawEZO() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(2);           // We are using a size multiplier of 1
  tft.setCursor(30, 10);    // Set cursor to x = 30, y = 175
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Set text colour to white and background to black
  tft.println(currentTime);
  
  //Title
  //tft.setFreeFont(&ArialRoundedMTBold_36);
  tft.setTextFont(2);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("Pool Monitor"));
  tft.drawString("Pool Monitor", 120, 200 - 2);

  //TEMP
  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(0); // Reset padding width to none
  tft.drawString("Temp ", 0, 240);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("-88.00`"));
  //if (TEMP_val.indexOf(".")) TEMP_val = TEMP_val.substring(0, TEMP_val.indexOf(".") + 1); // Make it .1 precision
  if (TEMP_val == "") TEMP_val = "?";  // Handle null return
  tft.drawString(TEMP_val + "`", 221, 240);
  tft.setTextDatum(BL_DATUM);
  tft.setTextPadding(0);
  //tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextFont(2);
  tft.drawString("C ", 221, 220);
  Blynk.virtualWrite (V1, TEMP_val);
  //tft.setTextDatum(MR_DATUM);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.setTextPadding(tft.textWidth("test"));
  //tft.drawString("test", 120, 240);

  //PH
  //tft.setFreeFont(&ArialRoundedMTBold_36);
  tft.setTextFont(2);
  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(0); // Reset padding width to none
  tft.drawString("PH", 0, 280);
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 3.777 "));
  tft.drawString(PH_val, 221, 280);
  Blynk.virtualWrite (V2, PH_val);

  //ORP
  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(0); // Reset padding width to none
  tft.drawString("ORP", 0, 315);
  //tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("200.3"));
  tft.drawString(ORP_val, 220, 315);
  tft.setTextDatum(BL_DATUM);
  //tft.setTextPadding(0);
  //tft.setFreeFont(&ArialRoundedMTBold_14);
  tft.setTextFont(2);
  tft.drawString("mV", 221, 310);
  Blynk.virtualWrite (V3, ORP_val);
  //Cleanup for next string
  tft.setTextPadding(0); // Reset padding width to none
}

/***********************************************************************************************
 * Atlas Scientific EZO code                                                                                     *     
 ***********************************************************************************************/
// do serial communication in a "asynchronous" way

void do_serial() {
  //if (millis() >= next_serial_time) {                // is it time for the next serial communication?
    for (int i = 0; i < TOTAL_CIRCUITS; i++) {       // loop through all the sensors
      Serial.print(channel_names[i]);                // print channel name
      Serial.print(":\t");
      Serial.println(readings[i]);                    // print the actual reading
      //Serial.println(i);
      PH_val = readings[1];
      ORP_val = readings[0];
      drawEZO();
      terminal.println(PH_val + "\n" + ORP_val + "\n");
      Serial.println(PH_val + " " + ORP_val);
    }
  //  next_serial_time = millis() + send_readings_every;
  //}
}


// take sensor readings in a "asynchronous" way
void do_sensor_readings() {
  if (request_pending) {                          // is a request pending?
    if (millis() >= next_reading_time) {          // is it time for the reading to be taken?
      receive_reading();                          // do the actual I2C communication
    }
  } else {                                        // no request is pending,
    channel = (channel + 1) % TOTAL_CIRCUITS;     // switch to the next channel (increase current channel by 1, and roll over if we're at the last channel using the % modulo operator)
    request_reading();                            // do the actual I2C communication
  }
}



// Request a reading from the current channel
void request_reading() {
  request_pending = true;
  Wire.beginTransmission(channel_ids[channel]); // call the circuit by its ID number.
  Wire.write('r');                    // request a reading by sending 'r'
  Wire.endTransmission();                   // end the I2C data transmission.
  next_reading_time = millis() + reading_delay; // calculate the next time to request a reading
}

void send_command() {
  request_pending = true;
  terminal.println(command_string);
  terminal.flush();
  Wire.beginTransmission(channel_ids[channel]);  // call the circuit by its ID number.
  Wire.write(command_string);                               // request a reading by sending command
  Wire.endTransmission();                        // end the I2C data transmission.
  next_reading_time = millis() + reading_delay;  // calculate the next time to request a reading
  terminal.println("Sending to probe, wait for reply\n");
  Blynk.run();
  delay(1000);
  Blynk.run();
  delay(1000);
  Blynk.run();
  terminal.println("Requesting reply\n");
  sensor_bytes_received = 0;                        // reset data counter
  memset(sensordata, 0, sizeof(sensordata));        // clear sensordata array;

  Wire.requestFrom(channel_ids[channel], 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
  //Wire.requestFrom(99, 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
  code = Wire.read();

  while (Wire.available()) {          // are there bytes to receive?
    in_char = Wire.read();            // receive a byte.

    if (in_char == 0) {               // if we see that we have been sent a null command.
      Wire.endTransmission();         // end the I2C data transmission.
      break;                          // exit the while loop, we're done here
    }
    else {
      sensordata[sensor_bytes_received] = in_char;  // load this byte into our array.
      sensor_bytes_received++;
    }
  }

  switch (code) {                       // switch case based on what the response code is.
    case 1:                             // decimal 1  means the command was successful.
      terminal.println("OK\n");
      terminal.flush();
      break;                              // exits the switch case.

    case 2:                             // decimal 2 means the command has failed.
      terminal.println("error: command failed\n");
      terminal.flush();
      break;                              // exits the switch case.

    case 254:                           // decimal 254  means the command has not yet been finished calculating.
      terminal.println("reading not ready\n");
      terminal.flush();
      break;                              // exits the switch case.

    case 255:                           // decimal 255 means there is no further data to send.
      terminal.println("error: no data\n");
      terminal.flush();
      break;                              // exits the switch case.
  }
  terminal.println("Continuing...\n");
  terminal.flush();
  request_pending = false;                  // set pending to false, so we can continue to the next sensor
}


// Receive data from the I2C bus
void receive_reading() {
  sensor_bytes_received = 0;                        // reset data counter
  memset(sensordata, 0, sizeof(sensordata));        // clear sensordata array;

  Wire.requestFrom(channel_ids[channel], 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
  code = Wire.read();

  while (Wire.available()) {          // are there bytes to receive?
    in_char = Wire.read();            // receive a byte.

    if (in_char == 0) {               // if we see that we have been sent a null command.
      Wire.endTransmission();         // end the I2C data transmission.
      break;                          // exit the while loop, we're done here
    }
    else {
      sensordata[sensor_bytes_received] = in_char;  // load this byte into our array.
      sensor_bytes_received++;
    }
  }

  switch (code) {                       // switch case based on what the response code is.
    case 1:                             // decimal 1  means the command was successful.
      readings[channel] = sensordata;
      break;                              // exits the switch case.

    case 2:                             // decimal 2 means the command has failed.
      readings[channel] = "error: command failed";
      break;                              // exits the switch case.

    case 254:                           // decimal 254  means the command has not yet been finished calculating.
      readings[channel] = "reading not ready";
      break;                              // exits the switch case.

    case 255:                           // decimal 255 means there is no further data to send.
      readings[channel] = "error: no data";
      break;                              // exits the switch case.
  }
  //Blynk.virtualWrite (V10, readings[channel]);
  request_pending = false;                  // set pending to false, so we can continue to the next sensor
}


/***********************************************************************************************
 * Temperature                                                                                      *     
 ***********************************************************************************************/
//Receive data from OneWire sensor
void requestTemp()  {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus

  Serial.print("Requesting temperature(s)...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  Serial.print("Temperature for Device 1 is: ");
  Serial.print(sensors.getTempCByIndex(0)); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  Serial.println();
  terminal.print(currentTime);
  terminal.print(", temp: ");
  terminal.print(sensors.getTempCByIndex(0));
  terminal.print("\n");
  TEMP_val = sensors.getTempCByIndex(0);
}
/***********************************************************************************************
 * Blynk                                                                                       *     
 ***********************************************************************************************/

 
BLYNK_WRITE(V11){
   cmdCalORP = param.asInt(); // Get the state of the VButton
   if (cmdCalORP == 1) {
    channel = 0;
    strcpy(command_string, ScmdCalORP);
    //send_command();
   }
}
BLYNK_WRITE(V12){
    cmdCalPH7 = param.asInt(); // Get the state of the VButton
    if (cmdCalPH7 == 1) {
    channel = 1;
    strcpy(command_string, ScmdCalPH7);
    //send_command();  
      }
} 
BLYNK_WRITE(V13){
    cmdCalPH4 = param.asInt(); // Get the state of the VButton
    if (cmdCalPH4 == 1) {
    channel = 1;
    strcpy(command_string, ScmdCalPH4);
    //send_command();  
      }
} 

BLYNK_WRITE(V15){
    cmdCalCORP = param.asInt(); // Get the state of the VButton
    if (cmdCalCORP == 1) {
    channel = 1;
    strcpy(command_string, ScmdCalCORP);
    //send_command();  
      }
} 
BLYNK_WRITE(V16){
    cmdCalCPH = param.asInt(); // Get the state of the VButton
    if (cmdCalCPH == 1) {
    channel = 1;
    strcpy(command_string, ScmdCalCPH);
    //send_command();  
      }
} 
BLYNK_WRITE(V17){
    cmdTempComp = param.asInt(); // Get the state of the VButton
    if (cmdTempComp == 1) {
    channel = 1;
    strcat( ScmdTempComp, TEMP_val.c_str() );
    strcpy(command_string, ScmdTempComp);
    terminal.print("Sending temp to compensate: ");
    terminal.print(TEMP_val);
    terminal.print("\n");
    //send_command();  
      }
} 
BLYNK_WRITE(InternalPinRTC) {
  long t = param.asLong();
  Serial.print("Synced RTC, Unix time: ");
  Serial.print(t);
  Serial.println();
  setTime(t);
}

/***********************************************************************************************
 * ====== RUN THIS CODE ONCE WHEN BLYNK IS FIRST CONNECTED ========                            *                                                            *     
 ***********************************************************************************************/

bool isFirstConnect = true;

BLYNK_CONNECTED() {
 if (isFirstConnect) {
      Blynk.virtualWrite (V4, 1);
    isFirstConnect = false;
    }
}

//===== RE CONNECT IF BLYNK NOT CONNECTED =====
void reconnectBlynk() {
  if (!Blynk.connected()) {
     Serial.println ("Blyn.connected is FALSE"); // for debugging
    if(Blynk.connect(3333)) {
      bool isFirstConnect = true; // assume this is first time we connected to Blynk again :-) i.e. ensure BLYNK_CONNECTED()to executed again, now that we re-connected
    } else {
      // nothing to do
    }
  }
}

//void RestartAPconfig() {
//   if (digitalRead(D4) == LOW ) {   // if PIN D4 is set to GND it will initiate WifiManager Captive portal - but you need to select "OnDemandAP" from your smartphone wifi settings
//    //WiFiManager
//    //Local intialization. Once its business is done, there is no need to keep it around
//    WiFiManager wifiManager;
//
//    //reset settings - for testing
//    wifiManager.resetSettings();  // needed to clear previous wifi settings help in non volatile ram
//
//    //sets timeout until configuration portal gets turned off
//    //useful to make it all retry or go to sleep
//    //in seconds
//    //wifiManager.setTimeout(120);
//
//    //it starts an access point with the specified name
//    //here  "AutoConnectAP"
//    //and goes into a blocking loop awaiting configuration
//
//    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
//    //WiFi.mode(WIFI_STA);
//    
//    if (!wifiManager.startConfigPortal("OnDemandAP")) {
//      Serial.println("failed to connect and hit timeout");
//      delay(3000);
//      //reset and try again, or maybe put it to deep sleep
//      ESP.reset();
//      delay(5000);
//    }
//    //if you get here you have connected to the WiFi
//    Serial.println("connected...yeey :)");
//  }
//
//}

 /***********************************************************************************************
 * Main Loop       *                                                                           *     
 *************************************************************************************************/
void loop() {
  // is configuration portal requested?
  
  
 
 if (Blynk.connected()) {
      Blynk.run(); }
  
  timer.run();
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
}

