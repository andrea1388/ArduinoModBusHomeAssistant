#include <Arduino.h>
#include <ModbusRtu.h> // https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino
// #include <OneWire.h> // https://github.com/PaulStoffregen/OneWire
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <jled.h>

#define DEBUG
#define SERIALSPEED 19200 
#define ID 1
#define PIN485TXEN 6
#define INPUTPINS {2} // list of input pins, MAX 16!!
#define OUTPUTPINS {3} // list of output pins, MAX 16!!
#define LIGHTOUTPUTPINS {10} // list of light output pins, MAX 16!!
#define LIGHTCONTROLPINS {8} // must be the same number of pins of LIGHTOUTPUTPINS array
// Modbus registers definition
#define TARGET_TEMP_REGISTER_LOW 0
#define TARGET_TEMP_REGISTER_HIGH 1
#define CURRENT_TEMP_REGISTER_LOW 2
#define CURRENT_TEMP_REGISTER_HIGH 3
#define BINARY_INPUTS 4
#define BINARY_OUTPUTS 5
#define BINARY_OUTPUTS_LIGHTS 6


// global objects
Modbus slave(ID, 0, PIN485TXEN);
uint16_t modbusregisters[9];
uint8_t inputpins[]=INPUTPINS;
uint8_t outputpins[]=OUTPUTPINS;
uint8_t lightoutputpins[]=LIGHTOUTPUTPINS;
uint8_t lightcontrolpins[]=LIGHTCONTROLPINS;
uint8_t inputnum=  sizeof(inputpins) / sizeof(uint8_t);
uint8_t outputnum=  sizeof(outputpins) / sizeof(uint8_t);
uint8_t lightnum=  sizeof(outputpins) / sizeof(uint8_t);
JLed led = JLed(LED_BUILTIN).Blink(50,50).Repeat(5);
Button *lightcontrolbutton[16];
Adafruit_SSD1306 display(128, 32, &Wire, -1);


// function prototypes
void DoWriteOutputRegister();
void DoResetInputRegister();
void DoUpdateDisplay();
void DoLightControl();
float readds();


void setup() {
  slave.begin(SERIALSPEED);
  for(uint8_t i=0;i<inputnum;i++)
    pinMode(inputpins[i],INPUT_PULLUP);
  for(uint8_t i=0;i<lightnum;i++)
    pinMode(lightcontrolpins[i],INPUT_PULLUP);
  for(uint8_t i=0;i<outputnum;i++)
    pinMode(outputpins[i],OUTPUT);
  for(uint8_t i=0;i<lightnum;i++)
    pinMode(lightoutputpins[i],OUTPUT);
  for(uint8_t i=0;i<lightnum;i++)
  {
    lightcontrolbutton[i]=new Button(lightcontrolpins[i]);
    lightcontrolbutton[i]->begin();
  }
  DoWriteOutputRegister();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.setTextSize(2); 
  display.setTextColor(WHITE);
  display.setCursor(0, 0); 
  display.print("Started");
  display.display();
  #ifdef DEBUG
    Serial.println("Started");
    Serial.print("input pins: ");
    for(uint8_t i=0;i<inputnum;i++)
    {
      Serial.print(inputpins[i]);
      Serial.print("/");
    }
    Serial.println("");
  #endif
}





// called after a poll
void polled() {
  DoWriteOutputRegister();
  DoResetInputRegister();
  led.Blink(50,50);
}

bool DoModBus() {
  int8_t state;
  state = slave.poll( modbusregisters, 9 );
  if (state > 4) {
    polled();
  }
}

void DoReadInputs()
{
  // read input pins and if reads 0 (button pressed) set the corresponding bit on the register
  // it will be cleared after poll if it's back to 1
  // this is because master poll is slow
  // state is inverted because of the pullup, so when button is pressed the register will get 1 on the corresponding bit
  for(uint8_t i=0;i<inputnum;i++)
    if(!digitalRead( inputpins[i] )) bitSet(modbusregisters[BINARY_INPUTS],i);
  for(uint8_t i=0;i<lightnum;i++)
    lightcontrolbutton[i]->read();
}

void DoResetInputRegister() {
  for(uint8_t i=0;i<inputnum;i++)
    if(digitalRead( inputpins[i] )) bitClear(modbusregisters[BINARY_INPUTS],0);
}


void DoWriteOutputRegister() {
  for(uint8_t i=0;i<outputnum;i++)
    digitalWrite( outputpins[i], bitRead( modbusregisters[BINARY_OUTPUTS], i ));
  for(uint8_t i=0;i<lightnum;i++)
    digitalWrite( lightoutputpins[i], bitRead( modbusregisters[BINARY_OUTPUTS_LIGHTS], i ));
}

void DoThermostat() {
  const float t=(float)*(modbusregisters+TARGET_TEMP_REGISTER_HIGH);

}

void DoReadTemperature() {
  float t=readds();
  memcpy(modbusregisters+TARGET_TEMP_REGISTER_HIGH,&t,4);

}

void loop() {
  DoReadTemperature();
  DoReadInputs();
  DoLightControl();
  DoModBus();
  DoThermostat();
  DoUpdateDisplay();
  led.Update();
}

void DoUpdateDisplay() {

}

void DoLightControl() {
  for(uint8_t i=0;i<lightnum;i++)
    if(lightcontrolbutton[i]->wasPressed()) {
      bitWrite(modbusregisters[BINARY_OUTPUTS_LIGHTS],i,!bitRead(modbusregisters[BINARY_OUTPUTS_LIGHTS],i));
      //Serial.println(modbusregisters[BINARY_OUTPUTS_LIGHTS]);
      DoWriteOutputRegister();
    }
}

float readds() {
  float f=20.1;
  return f;
}