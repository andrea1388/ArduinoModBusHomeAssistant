#include <Arduino.h>
#include <ModbusRtu.h> // https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino
#include <OneWire.h> // https://github.com/PaulStoffregen/OneWire

#define SERIALSPEED 19200 
#define ID 1
#define PIN485TXEN 6
#define INPUTPINS {2,3}
#define BINARY_INPUTS 4

// global objects
Modbus slave(ID, 0, PIN485TXEN);
uint16_t modbusregisters[9];
uint8_t inputpins[]=INPUTPINS;
uint8_t inputnum=  sizeof(inputpins) / sizeof(uint8_t);

void setup() {
  slave.begin(SERIALSPEED);
  for(uint8_t i=0;i<inputnum;i++)
    pinMode(inputpins[i],INPUT_PULLUP);
}

void loop() {
  DoReadTemperature();
  DoReadInputs();
  if (DoModBus()) {
    DoWriteOutputRegister();
    DoResetInputRegister();
  }
  DoThermostat();
  DoUpdateDisplay();
  
  
}

bool DoModBus() {
  int8_t state;
  state = slave.poll( modbusregisters, 9 );
  if (state > 4) {
    flashledpoll();
  }
}

void regtoio()
{
  // read input pins and if reads 0 set the corresponding bit on the register
  // it will be cleared after poll if it's back to 1
  // this is because master poll is slow
  // state is inverted because of the pullup, so when button is pressed the register will get 1 on the corresponding bit
  for(uint8_t i=0;i<inputnum;i++)
    if(!digitalRead( inputpins[i] )) bitSet(modbusregisters[BINARY_INPUTS],i);
}

void reset_binary_inputs_register() {
  for(uint8_t i=0;i<inputnum;i++)
    if(digitalRead( inputpins[i] )) bitClear(modbusregisters[BINARY_INPUTS],0);
}