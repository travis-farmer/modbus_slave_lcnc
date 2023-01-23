/*
  The master/client port will need to be configured using the following settings:
  - Baud Rate: 9600
  - Data Bits: 8
  - Parity: None
  - Stop Bit(s): 1
  - Slave/Server ID: 1
*/

#include <ModbusRTUSlave.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,40,2);

const byte id = 1;
const int dePin = 12;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 1;
const unsigned int numDiscreteInputs = 1;
const unsigned int numHoldingRegisters = 1;
const unsigned int numInputRegisters = 16;

unsigned long lastDispTim = 0UL;
String GRBLreturn = "";
String gblGRBLstatus = "";
int gblToolOut = 0;
int gblToolIn = 0;
bool GRBLinErr = false;
bool GRBLisHoming = false;
long gblStatus = 0;
int macStatus = 0;

byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

void setMachine(word stat) {
    macStatus = (int)stat;
}

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

void homeGRBL() {
    Serial2.println("$H");
    GRBLisHoming = true;
}

void Disp() {
    lcd.clear();
    char buffer[40];
    sprintf(buffer, "LCNC: %d Tool Out: %d Tool In: %d", macStatus, gblToolOut, gblToolIn);
    lcd.setCursor(0,0);
    lcd.print(buffer);
    char bufferb[40];
    sprintf(bufferb, "GRBL: %d", gblStatus);
    lcd.setCursor(0,1);
    lcd.print(bufferb);

}

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
}

void loop() {
  modbus.poll();

  unsigned long timTimer = millis();
  if ((timTimer - lastDispTim) >= 1000) {
    lastDispTim = timTimer;
    Disp();
    
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
}



char coilRead(unsigned int address) {
    // mb2hal: ???

}

boolean coilWrite(unsigned int address, boolean value) {
    // mb2hal: fnct_15_write_multiple_coils
    /*
    iocontrol.0.tool-change
    (Bit, Out) TRUE when a tool change is requested.

    iocontrol.0.tool-prepare
    (Bit, Out) TRUE when a Tn tool prepare is requested.

    */

  return true;
}

char discreteInputRead(unsigned int address) {
    // mb2hal: fnct_02_read_discrete_inputs

}

long holdingRegisterRead(unsigned int address) {
    // mb2hal: fnct_03_read_holding_registers
    switch(address) {
    case 0:
        return gblStatus;
        break;

    }
}

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
        setMachine(value);
        break;
    }
  return true;
}

long inputRegisterRead(word address) {
    // mb2hal: fnct_04_read_input_registers
  return analogRead(address);
}
