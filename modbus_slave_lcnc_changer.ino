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
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,40,2);

const int dePin = 12;
const int MacStatusPin = 22;
const int MacIsDonePin = 23;

const byte id = 1;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 3;
const unsigned int numDiscreteInputs = 1;
const unsigned int numHoldingRegisters = 2;
const unsigned int numInputRegisters = 16;

unsigned long lastDispTim = 0UL;
unsigned long lastWatchDog = 0UL;

String GRBLreturn = "";
String gblGRBLstatus = "";
int gblToolOut = 0;
int gblToolIn = 0;
int gblDwellStatus = 0;

unsigned int gblErrorCode = 0;
unsigned int gblLastDispPage = 0;

bool gblChangeRequested = false;
bool gblPrepareRequested = false;
bool feedWatchDog = false;
bool oldWatchDog = false;

char gblChangeReady = '0';
char gblPrepareReady = '0';
char WatchDogStatus = '0';

bool GRBLinErr = false;
bool GRBLisHoming = false;
long gblStatus = 0;
int macStatus = 0;

byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

/********************************************//**
 * \brief try to parse the return from GRBL
 *
 * \param inStr String
 * \return void
 *
 ***********************************************/
void parseGRBL(String inStr) {
    gblGRBLstatus = inStr;
    Disp();
    if (inStr == "ok") {
        GRBLinErr = false;
        GRBLisHoming = false;
        bitWrite(gblStatus,1,0);
    } else if (inStr.startsWith("ALARM:") == true) {
        //TODO: have a button to press to manually clear the error should re-home GRBL after an error
        Serial2.println("$X"); // for now, we auto clear it
        GRBLinErr = true;
        bitWrite(gblStatus,1,1);
    }
}

/********************************************//**
 * \brief move GRBL to an X-Y location
 *
 * \param X float
 * \param Y float
 * \return void
 *
 ***********************************************/
void goXY(float X, float Y) {
    // these are rapid moves. get there as fast as possible.
    char buffX[30];
    dtostrf(X, 1, 6, buffX);
    char buffY[30];
    dtostrf(Y, 1, 6, buffY);
    char buffer[65];
    sprintf(buffer, "G0 X%s Y%s", buffX, buffY);
    Serial2.println(buffer);
}

/********************************************//**
 * \brief Move GRBL to a Z location
 *
 * \param Z float
 * \param F float
 * \return void
 *
 ***********************************************/
void goZ(float Z, float F) {
    // feed rate move, allows Z to move more controlled.
    char buffZ[30];
    dtostrf(Z, 1, 6, buffZ);
    char buffF[30];
    dtostrf(F, 1, 6, buffF);
    char buffer[65];
    sprintf(buffer, "G1 X%s F%s", buffZ, buffF);
    Serial2.println(buffer);
}

/********************************************//**
 * \brief Home the GRBL axis
 *
 * \return void
 *
 ***********************************************/
void homeGRBL() {
    Serial2.println("$H");
    GRBLisHoming = true;
}

/********************************************//**
 * \brief Display status on a 4002 I2C LCD
 *
 * \return void
 *
 ***********************************************/
void Disp() {
    lcd.clear();
    char buffer[40];
    sprintf(buffer, "LCNC: %d Tool Out: %d Tool In: %d DW: %d", macStatus, gblToolOut, gblToolIn, gblDwellStatus);
    lcd.setCursor(0,0);
    lcd.print(buffer);

    char bufferb[40];
    switch(gblLastDispPage) {
    case 0:
        sprintf(bufferb, "ModBus ATC Mag v%s %s", AutoVersion::FULLVERSION_STRING, AutoVersion::STATUS);
        break;
    case 1:
        String tmpStr = "";
        tmpStr = String(gblErrorCode,HEX);
        char bufferErr[8];
        tmpStr.toCharArray(bufferErr, 8);
        sprintf(bufferb, "Error: 0x%sH", bufferErr);
        break;

    }
    gblLastDispPage++;
    if (gblLastDispPage > 1) {gblLastDispPage = 0;}
    lcd.setCursor(0,1);
    lcd.print(bufferb);

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

  Serial1.begin(baud);
  Serial2.begin(115200);
  modbus.begin(id, baud);
  modbus.configureCoils(numCoils, coilRead, coilWrite);
  modbus.configureDiscreteInputs(numDiscreteInputs, discreteInputRead);
  modbus.configureHoldingRegisters(numHoldingRegisters, holdingRegisterRead, holdingRegisterWrite);
  modbus.configureInputRegisters(numInputRegisters, inputRegisterRead);

  pinMode(MacStatusPin,INPUT_PULLUP);
  pinMode(MacIsDonePin,INPUT_PULLUP);
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

  if (Serial2.available() > 0) {
    // get incoming byte:
    char inChar = Serial2.read();
    if (inChar == "\n") {
        parseGRBL(GRBLreturn);
        GRBLreturn = "";
    } else if (inChar == "\r") {;}
    else {
        GRBLreturn += inChar;
    }
  }
  if (gblChangeRequested == true && gblToolIn > 0) {
    //TODO: move transfer arm into change location (if not already)
    gblChangeReady = '1';
    gblChangeRequested = false;
    while(digitalRead(MacIsDonePin) == HIGH) {
        //DWELL until machine is done loading tool into tool-in pocket and taking tool from tool-out pocket
        gblDwellStatus = 1;
    }
    gblDwellStatus = 0;
    //TODO: move transfer arm back to mag
  }
  if (gblPrepareRequested == true && gblToolOut > 0) {
    //TODO: grab tool from magazine, and load tool into transfer arm, tool out pocket
    //TODO: remove tool from tool-in pocket, and move back to mag
    gblPrepareReady = '1';
    gblPrepareRequested = false;

  }


  // poll machine status input pin
  //macStatus = !digitalRead(MacStatusPin);
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
        return gblChangeReady;
        break;
    case 1:
        return gblPrepareReady;
        break;
    case 2:
        return WatchDogStatus;
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
    /*
    iocontrol.0.tool-change
    (Bit, Out) TRUE when a tool change is requested.

    iocontrol.0.tool-prepare
    (Bit, Out) TRUE when a Tn tool prepare is requested.

    */
    switch(address) {
    case 0:
        gblChangeRequested = value;
        break;
    case 1:
        gblPrepareRequested = value;
        break;
    case 2:
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
    /*
    iocontrol.0.tool-number
    (s32, Out) Current tool number.

    iocontrol.0.tool-prep-number
    (s32, Out) The number of the next tool, from the RS274NGC T-word.
    */
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
