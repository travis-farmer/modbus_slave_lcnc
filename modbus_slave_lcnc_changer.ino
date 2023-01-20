/*
  The master/client port will need to be cofigured using the following settings:
  - Baud Rate: 9600
  - Data Bits: 8
  - Parity: None
  - Stop Bit(s): 1
  - Slave/Server ID: 1
  - Coils: 0 and 1
  - Discrete Inputs: 0 and 1
  - Holding Registers: 0 and 1
  - Input Registers: 0 and 1


*/

  /*
TODO:








  */

#include <ModbusRTUSlave.h>

const byte id = 1;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 2;
const unsigned int numDiscreteInputs = 2;
const unsigned int numHoldingRegisters = 2;
const unsigned int numInputRegisters = 16;

const byte buttonPins[numDiscreteInputs] = {2, 3};
const byte tonePin = 8;
const byte pwmPin = 9;

const byte dePin = 12;
const byte ledPin = 13;
const byte potPins[numInputRegisters] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15};

byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

byte dutyCycle = 0;
boolean toneActive = 0;
unsigned int toneFrequency = 0;

void setup() {
  pinMode(buttonPins[0], INPUT_PULLUP);
  pinMode(buttonPins[1], INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);
  pinMode(tonePin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(potPins[0], INPUT);
  pinMode(potPins[1], INPUT);

  Serial1.begin(baud);
  modbus.begin(id, baud);
  modbus.configureCoils(numCoils, coilRead, coilWrite);
  modbus.configureDiscreteInputs(numDiscreteInputs, discreteInputRead);
  modbus.configureHoldingRegisters(numHoldingRegisters, holdingRegisterRead, holdingRegisterWrite);
  modbus.configureInputRegisters(numInputRegisters, inputRegisterRead);
}

void loop() {
  modbus.poll();
  if (toneActive == true and toneFrequency > 0) {
    tone(tonePin, toneFrequency);
  }
  else {
    noTone(tonePin);
  }
}



char coilRead(unsigned int address) {
    // mb2hal: ???
  switch (address) {
    case 0:
      return digitalRead(ledPin);
    case 1:
      return toneActive;
  }
}

boolean coilWrite(unsigned int address, boolean value) {
    // mb2hal: fnct_15_write_multiple_coils
    /*
    iocontrol.0.tool-change
    (Bit, Out) TRUE when a tool change is requested.

    iocontrol.0.tool-prepare
    (Bit, Out) TRUE when a Tn tool prepare is requested.

    */
  switch (address) {
    case 0:
      digitalWrite(ledPin, value);
      break;
    case 1:
      toneActive = value;
      break;
  }
  return true;
}

char discreteInputRead(unsigned int address) {
    // mb2hal: fnct_02_read_discrete_inputs
    /*
    iocontrol.0.tool-changed
    (Bit, In) Should be driven TRUE when a tool change is completed.

    iocontrol.0.tool-prepared
    (Bit, In) Should be driven TRUE when a tool prepare is completed.
    */
  return !digitalRead(buttonPins[address]);
}

long holdingRegisterRead(unsigned int address) {
    // mb2hal: fnct_03_read_holding_registers
    /*
    iocontrol.0.tool-number
    (s32, Out) Current tool number.

    iocontrol.0.tool-prep-number
    (s32, Out) The number of the next tool, from the RS274NGC T-word.

    iocontrol.0.tool-prep-pocket
    (s32, Out) This is the pocket number (location in the tool storage mechanism) of the tool requested by the most recent T-word.
    */
  switch (address) {
    case 0:
      return dutyCycle;
    case 1:
      return toneFrequency;
  }
}

boolean holdingRegisterWrite(word address, word value) {
    // mb2hal: fnct_16_write_multiple_registers
  switch (address) {
    case 0:
      dutyCycle = constrain(value, 0, 255);
      analogWrite(pwmPin, dutyCycle);
      break;
    case 1:
      toneFrequency = 0;
      if (value >= 31) {
        toneFrequency = value;
      }
      break;
  }
  return true;
}

long inputRegisterRead(word address) {
    // mb2hal: fnct_04_read_input_registers
  return analogRead(potPins[address]);
}
