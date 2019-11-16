// INSTRUCTIONS: Copy MorphosesConfig.h.default to MorphosesConfig.h 
// and adjust according to your own Wifi setup.
// 
/* 20170116 add /motor/2 position with i2c send*/
/* 20161205 add /motor/1 speed with i2c send*/
/* mp_esp8266MPU9250 was */
/* MPU9250 Basic Example Code
 by: Kris Winer
 date: April 1, 2014
 license: Beerware - Use this code however you'd like. If you
 find it useful you can buy me a beer some time.
 Modified by Brent Wilkins July 19, 2016

 Demonstrate basic MPU-9250 functionality including parameterizing the register
 addresses, initializing the sensor, getting properly scaled accelerometer,
 gyroscope, and magnetometer data out. Added display functions to allow display
 to on breadboard monitor. Addition of 9 DoF sensor fusion using open source
 Madgwick and Mahony filter algorithms. Sketch runs on the 3.3 V 8 MHz Pro Mini
 and the Teensy 3.1.

 SDA and SCL should have external pull-up resistors (to 3.3V).
 10k resistors are on the EMSENSR-9250 breakout board.
 2.2k resistors are on the ESP8266Thing

 Hardware setup:
 MPU9250 Breakout --------- ESP8266Thing
 VDD ---------------------- 3.3V
 VDDI --------------------- 3.3V
 SDA ----------------------- SDA
 SCL ----------------------- SCL
 GND ---------------------- GND
 */

#include <OSCBundle.h>

#include <EEPROM.h>

#include <Wire.h>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial);

#include "MorphosesConfig.h"

OSCBundle bndl;
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

IPAddress destIP(DEST_IP_0, DEST_IP_1, DEST_IP_2, DEST_IP_3); // remote IP
float magScale[3] = {0, 0, 0};

char packetBuffer[128];

void setup()
{
  pinMode(power, OUTPUT);
  digitalWrite(power, HIGH);

  Wire.begin();

  // TWBR = 12;  // 400 kbit/sec I2C speed
  Serial.begin(115200);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  
#if MAIN_BOARD
  // Set up the interrupt pin, it's set as active high, push-pull
  pinMode(intPin, INPUT); // interrupt out from the IMU
  digitalWrite(intPin, LOW);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  digitalWrite(redLed, HIGH);
  digitalWrite(greenLed, HIGH);
  digitalWrite(blueLed, HIGH);
#endif

	// Initialize Wifi and UDP.
	initWifi();

	// Initialize and calibrate IMU.
  EEPROM.begin(512);

#if MAIN_BOARD
  digitalWrite(blueLed, LOW);
#endif
}



void loop()
{
    // if there's data available, read a packet
  int packetSize = udp.parsePacket();

  if (packetSize)
  {
    if (OSCDebug) {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
    }
    IPAddress remote = udp.remoteIP();
    if (OSCDebug) {
      for (int i = 0; i < 4; i++)
      {
        Serial.print(remote[i], DEC);
        if (i < 3)
        {
          Serial.print(".");
        }
      }
      Serial.print(", port ");
      Serial.println(udp.remotePort());
    }

    // Read the packet
    OSCMessage messIn;
    while (packetSize--) messIn.fill(udp.read());

    switch(messIn.getError()) {
      case  OSC_OK:
        int messSize;
        if (OSCDebug) Serial.println("no errors in packet");
        messSize = messIn.size();
        if (SerialDebug) {
          Serial.print("messSize: ");
          Serial.println(messSize);
        }
        if (OSCDebug) {
          char addressIn[64];
          messSize = messIn.getAddress(addressIn, 0, 64);
          Serial.print("messSize: ");
          Serial.println(messSize);
          Serial.print("address: ");
          Serial.println(addressIn);
        }
        processMessage(messIn);

        break;
      case BUFFER_FULL:
        if (OSCDebug) Serial.println("BUFFER_FULL error");
        break;
      case INVALID_OSC:
        if (OSCDebug) Serial.println("INVALID_OSC error");
        break;
      case ALLOCFAILED:
        if (OSCDebug) Serial.println("ALLOCFAILED error");
        break;
      case INDEX_OUT_OF_BOUNDS:
        if (OSCDebug) Serial.println("INDEX_OUT_OF_BOUNDS error");
        break;
    }
  } //if (packetSize)


#if MAIN_BOARD
	// Read motors. --> for now this is included in processIMU(), see NOTE below
	processMotors();
#endif

	// Send bundle.
  if (sendOSC) {

    if (useUdp) {
      udp.beginPacket(destIP, destPort);
      bndl.send(udp); // send the bytes to the SLIP stream
      udp.endPacket(); // mark the end of the OSC Packet
    }
    else {
      SLIPSerial.beginPacket();
      bndl.send(udp); // send the bytes to the SLIP stream
      SLIPSerial.endPacket(); // mark the end of the OSC Packet
    }
    bndl.empty(); // empty the bundle to free room for a new one
  }
}

#if MAIN_BOARD
void processMotors()
{
	// get the motor 1 encoder count
	byte incomingCount = Wire.requestFrom((uint8_t)MOTOR1_I2C_ADDRESS, (uint8_t)4);    // request 4 bytes from slave device #8
	byte tick3 = Wire.read();
	byte tick2 = Wire.read();
	byte tick1 = Wire.read();
	byte tick0 = Wire.read();
	if(SerialDebug) {
	  Serial.print("received ");
	  Serial.print(incomingCount);
	  Serial.print(": (3)");
	  Serial.print(tick3, HEX);
	  Serial.print("(2)");
	  Serial.print(tick2, HEX);
	  Serial.print("(1)");
	  Serial.print(tick1, HEX);
	  Serial.print("(0)");
	  Serial.println(tick0, HEX);
	}
	int32_t motor1Ticks = (tick3<<24) + (tick2<<16) + (tick1<<8) + tick0;
	if (sendOSC) bndl.add("/motor/1/ticks").add(motor1Ticks);

	// get the motor 2 encoder count
	incomingCount = Wire.requestFrom((uint8_t)MOTOR2_I2C_ADDRESS, (uint8_t)4);    // request 4 bytes from slave device #16
	tick3 = Wire.read();
	tick2 = Wire.read();
	tick1 = Wire.read();
	tick0 = Wire.read();
	if(SerialDebug) {
	  Serial.print("received ");
	  Serial.print(incomingCount);
	  Serial.print(": (3)");
	  Serial.print(tick3, HEX);
	  Serial.print("(2)");
	  Serial.print(tick2, HEX);
	  Serial.print("(1)");
	  Serial.print(tick1, HEX);
	  Serial.print("(0)");
	  Serial.println(tick0, HEX);
	}
	int32_t motor2Ticks = (tick3<<24) + (tick2<<16) + (tick1<<8) + tick0;
	if (sendOSC) bndl.add("/motor/2/ticks").add(motor2Ticks);
}
#endif

void initWifi()
{
  /**
   * Set up an access point
   * @param ssid          Pointer to the SSID (max 63 char).
   * @param passphrase    (for WPA2 min 8 char, for open use NULL)
   * @param channel       WiFi channel number, 1 - 13.
   * @param ssid_hidden   Network cloaking (0 = broadcast SSID, 1 = hide SSID)
   */
  // now start the wifi
  WiFi.mode(WIFI_AP_STA);
#if AP_MODE
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Can't start softAP");
    while(1); // Loop forever if setup didn't work
  }

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

#else
  WiFi.begin(ssid, password);
  Serial.print("Connecting to access point: \"");
  Serial.print(ssid);
  Serial.println("\"");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress myIP = WiFi.localIP();
  Serial.println(myIP);
#endif

  digitalWrite(greenLed, LOW);


  Serial.println("Starting UDP");
  if (!udp.begin(localPort)) {
    Serial.println("Can't start UDP");
    while(1); // Loop forever if setup didn't work
  }
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  digitalWrite(redLed, LOW);

  WiFi.printDiag(Serial);
}

void waitForInputSerial() {
  while (!Serial.available()) delay(10);
  flushInputSerial();
}

void flushInputSerial() {
  while (Serial.available())
    Serial.read();
}

/// Smart-converts argument from message to integer.
int32_t getArgAsInt(OSCMessage& msg, int index) {
  if (msg.isInt(index))
    return msg.getInt(index);
  else if (msg.isBoolean(index))
    return (msg.getBoolean(index) ? 1 : 0);
  else {
    double val = 0;
    if (msg.isFloat(index))       val = msg.getFloat(index);
    else if (msg.isDouble(index)) val = msg.getDouble(index);
    return round(val);
  }
}

/// Returns true iff argument from message is convertible to a number.
boolean argIsNumber(OSCMessage& msg, int index) {
  return (msg.isInt(index) || msg.isFloat(index) || msg.isDouble(index) || msg.isBoolean(index));
}

void processMessage(OSCMessage& messIn) {
  if (messIn.fullMatch("/stream")) {
    if (OSCDebug) Serial.println("STREAM");
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("stream value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      sendOSC = (val != 0);
    }
  }
  else if (messIn.fullMatch("/replyto")) {
    if (OSCDebug) Serial.println("REPLYTO");
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("reply value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      destIP[3] = val;
    }
  }
#if MAIN_BOARD
  else if (messIn.fullMatch("/power")) {
    if (OSCDebug) Serial.println("POWER");
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("power value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      digitalWrite(power, val ? LOW : HIGH);
    }
  }
  else if (messIn.fullMatch("/motor/1")) {
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("motor 1 value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      char val8 = (char)(val&0xFF);
      Wire.beginTransmission(MOTOR1_I2C_ADDRESS); // transmit to device #8
      Wire.write(MOTOR_SPEED); // sends one byte
      Wire.write(val>>24); // send 4 bytes bigendian 32-bit int
      Wire.write(val>>16);
      Wire.write(val>>8);
      Wire.write(val);
      Wire.endTransmission(); // stop transmitting
    }
  }
  else if (messIn.fullMatch("/motor/2")) {
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("motor 2 value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      char val8 = (char)(val&0xFF);
      Wire.beginTransmission(MOTOR2_I2C_ADDRESS); // transmit to device #8
      Wire.write(MOTOR_POSITION); // sends one byte
      Wire.write(val>>24); // send 4 bytes bigendian 32-bit int
      Wire.write(val>>16);
      Wire.write(val>>8);
      Wire.write(val);
      Wire.endTransmission(); // stop transmitting
    }
  }
  else if (messIn.fullMatch("/reset/2")) {
    // no args
    if (OSCDebug) Serial.println("reset 2");
    Wire.beginTransmission(MOTOR2_I2C_ADDRESS); // transmit to device #8
    Wire.write(MOTOR_RESET); // sends one byte
    Wire.endTransmission(); // stop transmitting
  }
  else if (messIn.fullMatch("/red")) {
    if (OSCDebug) Serial.println("RED");
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      //digitalWrite(redLed, (val != 0));
      analogWrite(redLed, (val%256));
    }
  }
  else if (messIn.fullMatch("/green")) {
    if (OSCDebug) Serial.println("GREEN");
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      //digitalWrite(greenLed, (val != 0));
      analogWrite(greenLed, (val%256));
    }
  }
  else if (messIn.fullMatch("/blue")) {
    if (OSCDebug) Serial.println("BLUE");
    if (argIsNumber(messIn, 0)) {
      if (OSCDebug) Serial.print("value ");
      int32_t val = getArgAsInt(messIn, 0);
      if (OSCDebug) Serial.println(val);
      //digitalWrite(blueLed, (val != 0));
      analogWrite(blueLed, (val%256));
    }
  }
#endif
}
