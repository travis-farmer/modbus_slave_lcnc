/**
 * \brief Arduino ModBus project, controlling a automatic tool changer.
 * The master/client port will need to be configured using the following settings:
 * Baud Rate: 9600
 * Data Bits: 8
 * Parity: None
 * Stop Bit(s): 1
 * Slave/Server ID: 1
 */

#include "version.h"
#include <ModbusRTUSlave.h>
#include <SPI.h>
#include <Ethernet.h>
#include "arduino_secrets.h"
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_ADS1X15.h>

// Update these with values suitable for your hardware/network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress server(10, 150, 86, 185);
IPAddress ip(192, 168, 0, 29);
IPAddress myDns(192, 168, 0, 1);

LiquidCrystal_I2C lcd(0x27,40,2);

Adafruit_ADS1115 ads;

const int dePin = 12;

const byte id = 1;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 10;
const unsigned int numDiscreteInputs = 1;
const unsigned int numHoldingRegisters = 10;
const unsigned int numInputRegisters = 16;

const int PSIaPin=0;
const int PSIbPin=1;
const int PSIcPin=2;

unsigned long lastDispTim = 0UL;
unsigned long lastWatchDog = 0UL;


byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

void callback(char* topic, char* payload, unsigned int length) {
  //Serial.print(topic);
  //Serial.print(":");
  for (int i=0; i< length; i++) {
    //Serial.print(payload[i]);
  }
  //Serial.print(":");
  //Serial.println(length);
}

EthernetClient eClient;
PubSubClient client(eClient);

unsigned long lastReconnectAttempt = 0UL;

void Disp() {
    int PSIa = (int)map(((5/1024)*(float)analogRead(PSIaPin)),0.50,4.50,0.00,150.00);
    int PSIb = (int)map(((5/1024)*(float)analogRead(PSIbPin)),0.50,4.50,0.00,150.00);

    char sz[32];
    sprintf(sz, "%d", PSIa);
    client.publish("lcnc/psia",sz);
    sprintf(sz, "%d", PSIb);
    client.publish("lcnc/psib",sz);


}

boolean reconnect() {
  if (client.connect("arduinoClient")) {
    // Once connected, publish an announcement...
    // client.publish("test/outTopic","testing");
    // ... and resubscribe
    // client.subscribe("generator/Monitor/Platform_Stats/System_Uptime");
    Disp();

  }
  return client.connected();
}

/**
 * \brief Arduino Setup
 *
 * \return void
 *
 */
void setup() {
    WiFi.setPins(8,7,4); // CS, IRQ, RST
    Serial1.begin(baud);
    modbus.begin(id, baud);
    modbus.configureCoils(numCoils, coilRead, coilWrite);
    modbus.configureDiscreteInputs(numDiscreteInputs, discreteInputRead);
    modbus.configureHoldingRegisters(numHoldingRegisters, holdingRegisterRead, holdingRegisterWrite);
    modbus.configureInputRegisters(numInputRegisters, inputRegisterRead);
    ads.begin()

    for (int i = 22; i <= 31; i++) {
        pinMode(i,INPUT_PULLUP);
    }
    for (int j = 32; j <= 41; j++) {
        pinMode(j,OUTPUT);
    }
    for (int k = 2; k <= 11; k++) {
        pinMode(k,OUTPUT);
    }
    client.setServer(server, 1883);
    client.setCallback(callback);
    char buffer[40];
    if (Ethernet.begin(mac) == 0) {
        //Serial.println("Failed to configure Ethernet using DHCP");
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
            lcd.setCursor(0,1);
            sprintf(buffer, "No Hardware");
            lcd.print(buffer);
        }
        if (Ethernet.linkStatus() == LinkOFF) {
            //Serial.println("Ethernet cable is not connected.");
            lcd.setCursor(0,1);
            sprintf(buffer, "Cable Unplugged");
            lcd.print(buffer);
        }
    } else {
        //Serial.print("  DHCP assigned IP ");
        //Serial.println(Ethernet.localIP());
        lcd.setCursor(0,1);
        sprintf(buffer, "IP: %s (DHCP)", Ethernet.localIP());
        lcd.print(buffer);
    }

    lastReconnectAttempt = 0;
}

/**
 * \brief Arduino Loop
 *
 * \return void
 *
 */
void loop() {
    modbus.poll();
    unsigned long now = millis();
    if (!client.connected()) {
        if (now - lastReconnectAttempt > 5000UL) {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (reconnect()) {
                lastReconnectAttempt = 0UL;
            }
        }
    } else {
        // Client connected

        client.loop();
    }
    if (now-lastDispTim > 1000) {
        lastDispTim = now;
        Disp();
    }
}



/**
 * \brief ModBus Coil Read handle
 *
 * \param address unsigned int
 * \return char
 *
 */
char coilRead(unsigned int address) {
    // mb2hal: fnct_01_read_coils
    switch(address) {
    case 0:
        return digitalRead(22)? 0:1;
        break;
    case 1:
        return digitalRead(23)? 0:1;
        break;
    case 2:
        return digitalRead(24)? 0:1;
        break;
    case 3:
        return digitalRead(25)? 0:1;
        break;
    case 4:
        return digitalRead(26)? 0:1;
        break;
    case 5:
        return digitalRead(27)? 0:1;
        break;
    case 6:
        return digitalRead(28)? 0:1;
        break;
    case 7:
        return digitalRead(29)? 0:1;
        break;
    case 8:
        return digitalRead(30)? 0:1;
        break;
    case 9:
        return digitalRead(31)? 0:1;
        break;

    }
}

/**
 * \brief ModBus Coil Write Handle
 *
 * \param address unsigned int
 * \param value boolean
 * \return boolean
 *
 */
boolean coilWrite(unsigned int address, boolean value) {
    // mb2hal: fnct_05_write_single_coil

    switch(address) {
    case 0:
        digitalWrite(32,value);
        break;
    case 1:
        digitalWrite(33,value);
        break;
    case 2:
        digitalWrite(34,value);
        break;
    case 3:
        digitalWrite(35,value);
        break;
    case 4:
        digitalWrite(36,value);
        break;
    case 5:
        digitalWrite(37,value);
        break;
    case 6:
        digitalWrite(38,value);
        break;
    case 7:
        digitalWrite(39,value);
        break;
    case 8:
        digitalWrite(40,value);
        break;
    case 9:
        digitalWrite(41,value);
        break;


    }
  return true;
}

/**
 * \brief ModBus Discrete Input Read Handle
 *
 * \param address unsigned int
 * \return char
 *
 */
char discreteInputRead(unsigned int address) {
    // mb2hal: fnct_02_read_discrete_inputs

}

/**
 * \brief ModBus Holding Register Read Handle
 *
 * \param address unsigned int
 * \return long
 *
 */
long holdingRegisterRead(unsigned int address) {
    // mb2hal: fnct_03_read_holding_registers
    return ads.readADC_SingleEnded(address);
}

/**
 * \brief ModBus Holding Register Write Handle
 *
 * \param address word
 * \param value word
 * \return boolean
 *
 */
boolean holdingRegisterWrite(word address, word value) {
    // mb2hal: fnct_16_write_multiple_registers

    switch(address) {
    case 0:
        analogWrite(2,value);
        break;
    case 1:
        analogWrite(3,value);
        break;
    case 2:
        analogWrite(4,value);
        break;
    case 3:
        analogWrite(5,value);
        break;
    case 4:
        analogWrite(6,value);
        break;
    case 5:
        analogWrite(7,value);
        break;
    case 6:
        analogWrite(8,value);
        break;
    case 7:
        analogWrite(9,value);
        break;
    case 8:
        analogWrite(10,value);
        break;
    case 9:
        analogWrite(11,value);
        break;

    }
  return true;
}

/**
 * \brief ModBus Input Register Read Handle
 *
 * \param address word
 * \return long
 *
 */
long inputRegisterRead(word address) {
    // mb2hal: fnct_04_read_input_registers
  return analogRead(address);
}
