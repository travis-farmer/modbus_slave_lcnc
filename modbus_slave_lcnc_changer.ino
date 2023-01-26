/********************************************//**
 * \brief Arduino ModBus project, controlling a automatic tool changer.
 * The master/client port will need to be configured using the following settings:
 * Baud Rate: 9600
 * Data Bits: 8
 * Parity: None
 * Stop Bit(s): 1
 * Slave/Server ID: 1
 ***********************************************/

#include "version.h"
#include <ModbusRTUSlave.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,40,2);

// Update these with values suitable for your hardware/network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xEE };
IPAddress server(10, 150, 86, 185);
IPAddress ip(192, 168, 0, 29);
IPAddress myDns(192, 168, 0, 1);

const int dePin = 12;

const byte id = 1;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 1;
const unsigned int numDiscreteInputs = 1;
const unsigned int numHoldingRegisters = 2;
const unsigned int numInputRegisters = 16;

unsigned long lastDispTim = 0UL;
unsigned long lastWatchDog = 0UL;

bool feedWatchDog = false;
bool oldWatchDog = false;

char WatchDogStatus = '0';

int macStatus = 0;
int EthStat = 0;


byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

/********************************************//**
 * \brief PubSubClient callback
 *
 * \param topic char*
 * \param payload char*
 * \param length unsigned int
 * \return void
 *
 ***********************************************/
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

long lastReconnectAttempt = 0;

/********************************************//**
 * \brief Auto Reconnect
 *
 * \return boolean
 *
 ***********************************************/
boolean reconnect() {
  if (client.connect("arduinoClient")) {
    Disp();
    EthStat = 0;

  }
  return client.connected();
}

/********************************************//**
 * \brief Display status on a LCD and PubSubClient
 *
 * \return void
 *
 ***********************************************/
void Disp() {
    char buffer[40];
    lcd.setCursor(0,0);
    sprintf(buffer, "LCNC: %d Eth: %d", macStatus, EthStat);
    lcd.print(buffer);
    lcd.setCursor(0,1);
    char bufferB[40];

    sprintf(bufferB, "IP: %s", Ethernet.localIP());
    lcd.print(bufferB);

    char sz[32];
    sprintf(sz, "%d", macStatus);
    client.publish("lcnc/macstatus",sz);
}

/********************************************//**
 * \brief Arduino Setup
 *
 * \return void
 *
 ***********************************************/
void setup() {
  lcd.init();                      // initialize the lcd
  lcd.backlight();

  client.setServer(server, 1883);
  client.setCallback(callback);
 //Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      EthStat = 1;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      //Serial.println("Ethernet cable is not connected.");
      EthStat = 2;
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    //Serial.print("  DHCP assigned IP ");
    //Serial.println(Ethernet.localIP());
  }
  delay(1500);
  lastReconnectAttempt = 0;

  Serial1.begin(baud);
  Serial2.begin(115200);
  modbus.begin(id, baud);
  modbus.configureCoils(numCoils, coilRead, coilWrite);
  modbus.configureDiscreteInputs(numDiscreteInputs, discreteInputRead);
  modbus.configureHoldingRegisters(numHoldingRegisters, holdingRegisterRead, holdingRegisterWrite);
  modbus.configureInputRegisters(numInputRegisters, inputRegisterRead);
}

/********************************************//**
 * \brief Arduino Loop
 *
 * \return void
 *
 ***********************************************/
void loop() {
  modbus.poll();

  unsigned long timTimer = millis();
  if ((timTimer - lastDispTim) >= 1500) {
    lastDispTim = timTimer;
    Disp();

  }

  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }

  if (oldWatchDog != feedWatchDog && (timTimer - lastWatchDog) < 1000) {
    // watchdog not starving
    oldWatchDog = feedWatchDog;
    lastWatchDog = timTimer;
    WatchDogStatus = '0';
  } else if ((timTimer - lastWatchDog) >= 1000){
    // watchdog starving, hasn't eaten in over 1000ms
    WatchDogStatus = '1';
    macStatus = 0; // Machine is lost
  }
}



/********************************************//**
 * \brief ModBus Coil Read handle
 *
 * \param address unsigned int
 * \return char
 *
 ***********************************************/
char coilRead(unsigned int address) {
    // mb2hal: fnct_01_read_coils
    switch(address) {
    case 0:
        return WatchDogStatus;
        break;
    }
}

/********************************************//**
 * \brief ModBus Coil Write Handle
 *
 * \param address unsigned int
 * \param value boolean
 * \return boolean
 *
 ***********************************************/
boolean coilWrite(unsigned int address, boolean value) {
    // mb2hal: fnct_05_write_single_coil

    switch(address) {
    case 0:
        feedWatchDog = value;
        break;
    }
  return true;
}

/********************************************//**
 * \brief ModBus Discrete Input Read Handle
 *
 * \param address unsigned int
 * \return char
 *
 ***********************************************/
char discreteInputRead(unsigned int address) {
    // mb2hal: fnct_02_read_discrete_inputs

}

/********************************************//**
 * \brief ModBus Holding Register Read Handle
 *
 * \param address unsigned int
 * \return long
 *
 ***********************************************/
long holdingRegisterRead(unsigned int address) {
    // mb2hal: fnct_03_read_holding_registers
    switch(address) {
    case 0:
        return gblStatus;
        break;

    }
}

/********************************************//**
 * \brief ModBus Holding Register Write Handle
 *
 * \param address word
 * \param value word
 * \return boolean
 *
 ***********************************************/
boolean holdingRegisterWrite(word address, word value) {
    // mb2hal: fnct_16_write_multiple_registers

    switch(address) {
    case 0:
        gblToolIn = value;
        break;
    case 1:
        gblToolOut = value;
        break;
    }
  return true;
}

/********************************************//**
 * \brief ModBus Input Register Read Handle
 *
 * \param address word
 * \return long
 *
 ***********************************************/
long inputRegisterRead(word address) {
    // mb2hal: fnct_04_read_input_registers
  return analogRead(address);
}
