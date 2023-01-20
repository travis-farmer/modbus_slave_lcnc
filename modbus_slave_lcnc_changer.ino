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

#include <ModbusRTUSlave.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,40,2);

const float Y_Center_Spacing = 3.50;
const float X_Center_Spacing = 3.50;
const int Y_Num_Pockets = 10;
const int X_Num_Pockets = 10;
const float Y_Home_Loc = 0.00;
const float X_Home_Loc = 0.00;
const float Y_Change_Loc = 0.00;
const float X_Change_Loc = 24.00;
const float Z_Tool_Depth = 4.50;

const byte id = 1;
const unsigned long baud = 9600;
const unsigned int bufSize = 256;

const unsigned int numCoils = 3;
const unsigned int numDiscreteInputs = 2;
const unsigned int numHoldingRegisters = 3;
const unsigned int numInputRegisters = 16;

const byte buttonPins[numDiscreteInputs] = {2, 3};
const byte tonePin = 8;
const byte pwmPin = 9;

const byte dePin = 12;
const byte ledPin = 13;
const byte potPins[numInputRegisters] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15};

int gblStatusID = 0;
int gblToolOut = 0;
int gblToolFetched = 0;
int gblErrCode = 0;
char gblStatTxt[36] = "";
bool gblStatLcnc = false;
bool gblToolChange = false;
bool gblToolPrepare = false;
bool gblToolIsChanged = false;
bool gblToolIsPrepared = false;
bool gblToolFetchIsReturn = false;
long gblToolNumber = 0;
long gblToolPrepNumber = 0;
long gblToolPrepPocket = 0;



unsigned long lastDispTim = 0UL;

byte buf[bufSize];
ModbusRTUSlave modbus(Serial1, buf, bufSize, dePin);

byte dutyCycle = 0;
boolean toneActive = 0;
unsigned int toneFrequency = 0;

void Disp() {
    lcd.clear();
    char buffer[40];
    sprintf(buffer, "Err: %d Tool Out: %d Fetched: %d", gblErrCode, gblToolOut, gblToolFetched);
    lcd.setCursor(0,0);
    lcd.print(buffer);
    char bufferb[40];
    sprintf(bufferb, "%d: %s", gblStatusID, gblStatTxt);
    lcd.setCursor(0,1);
    lcd.print(bufferb);

}

void setStatTxt(String InStr) {
  InStr.toCharArray(gblStatTxt, 36);
}

void SetupStatus() {
  if (bitRead(gblStatusID,0) == 1) {
    digitalWrite(ledPin,HIGH);
    setStatTxt("LinuxCNC Enabled");
  } else if (bitRead(gblStatusID,0) == 0) {
    digitalWrite(ledPin,LOW);
    setStatTxt("Linuxcnc Disabled");
  }

}

void lcncMachineOn(bool MacStat) {
    // what to do when machine is turned on or off
    bitWrite(gblStatusID,0,MacStat? 1:0); // set status bit 0


}

void lcncChangeTool(bool ChngStat) {
    // what to do when a tool change is called
    if (gblToolIsPrepared == true && gblToolPrepNumber != gblToolNumber && gblToolFetchIsReturn != true) {
        // tool preped is not current tool, so continue with change

        String tmpStr = "Change from: ";
        tmpStr += gblToolNumber;
        tmpStr += " To: ";
        tmpStr += gblToolPrepNumber;
        setStatTxt(tmpStr);
        gblToolFetched = gblToolNumber;
        gblToolFetchIsReturn = true;
        gblToolNumber = gblToolPrepNumber;
        gblToolOut = gblToolNumber;
        gblToolIsChanged = true;

    } else {
        // something is wrong, assert an error
        bitWrite(gblErrCode,0,1);
        setStatTxt("Tool Num Mismatch");
    }
}

void lcncPrepTool(bool PrepStat) {
    if (gblToolIsPrepared != true) {
        if (gblToolFetchIsReturn == true) {
            // return tool first, before prefetch
            String tmpStr = "Returning: ";
            tmpStr += gblToolPrepNumber;
            setStatTxt(tmpStr);
            gblToolFetchIsReturn = false;
            gblToolPrepNumber = 0;// return is re-stocked in correct pocket, according to tool number
        }
        // prep the next tool
        String tmpStrB = "Prepped: ";
        tmpStrB += gblToolPrepNumber;
        setStatTxt(tmpStrB);
        gblToolPrepNumber = gblToolPrepNumber; // put in place as temporary
        gblToolFetched = gblToolPrepNumber;
        gblToolIsPrepared = true;
    }
}

void setup() {
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  pinMode(buttonPins[0], INPUT_PULLUP);
  pinMode(buttonPins[1], INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);
  pinMode(tonePin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(potPins[0], INPUT);
  pinMode(potPins[1], INPUT);

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
    SetupStatus();
  }
}



char coilRead(unsigned int address) {
    // mb2hal: ???
  switch (address) {
    case 0:
        // not sure why LCNC needs to know LCNC's status, but there you go...
      return gblStatLcnc;
    case 1:
      return gblToolChange;
    case 2:
        return gblToolPrepare;
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
        // LinuxCNC machine on atatus
      lcncMachineOn(value);
      break;
    case 1:
      lcncChangeTool(value);
      break;
    case 2:
       gblToolPrepare = value;
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
    switch (address) {
    case 0:
        return gblToolIsChanged;
        break;
    case 1:
        return gblToolIsPrepared;
        break;
    }
}

long holdingRegisterRead(unsigned int address) {
    // mb2hal: fnct_03_read_holding_registers
  switch (address) {
    case 0:
      return dutyCycle;
    case 1:
      return toneFrequency;
  }
}

boolean holdingRegisterWrite(word address, word value) {
    // mb2hal: fnct_16_write_multiple_registers
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
        if (value != gblToolNumber) {
            gblToolNumber = value;
            gblToolIsChanged = false;
        }

      break;
    case 1:
        if (value != gblToolPrepNumber) {
            gblToolPrepNumber = value;
            gblToolIsPrepared = false;
        }

      break;
    case 2:
        gblToolPrepPocket = value;
        break;
  }
  return true;
}

long inputRegisterRead(word address) {
    // mb2hal: fnct_04_read_input_registers
  return analogRead(potPins[address]);
}
