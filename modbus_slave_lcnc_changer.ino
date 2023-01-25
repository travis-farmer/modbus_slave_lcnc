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

byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

/********************************************//**
 * \brief Display status on a Nextion TFT
 *
 * \return void
 *
 ***********************************************/
void Disp() {
    char buffer[40];
    sprintf(buffer, "LCNC: %d Tool Out: %d Tool In: %d DW: %d", macStatus, gblToolOut, gblToolIn, gblDwellStatus);
    lcd.print(buffer);

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
