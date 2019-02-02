#include <Arduino.h>
#include <ModbusRtu.h> // https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino
#include <OneWire.h> // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <jled.h>

// #define DEBUG
#define SERIALSPEED 19200 
#define ID 1
#define PIN485TXEN 6
#define ONE_WIRE_BUS 2
#define INPUTPINS {4} // list of input pins, MAX 16!!
#define OUTPUTPINS {3} // list of output pins, MAX 16!!
#define LIGHTOUTPUTPINS {10} // list of light output pins, MAX 16!!
#define LIGHTCONTROLPINS {8} // must be the same number of pins of LIGHTOUTPUTPINS array
#define HEATHERCONTROPIN 11

// Modbus registers definition
#define TARGET_TEMP_REGISTER_HIGH 0
#define TARGET_TEMP_REGISTER_LOW 1
#define CURRENT_TEMP_REGISTER_HIGH 2
#define CURRENT_TEMP_REGISTER_LOW 3
#define BINARY_INPUTS 4
#define BINARY_OUTPUTS 5
#define BINARY_OUTPUTS_LIGHTS 6
#define ACMODE 7


// global objects
Modbus slave(ID, 0, PIN485TXEN);
uint16_t modbusregisters[8];
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
//float current_temperature, target_temperature;
bool update_display; // if set to true cause the display to be updated
bool ACMode=false;
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

// function prototypes
void DoWriteOutputRegister();
void DoResetInputRegister();
void DoUpdateDisplay();
void DoLightControl();
void ReadTemperaureSensor();
void DoProcessOtherRegisters();

void setup() {
  slave.begin(SERIALSPEED);
  pinMode(HEATHERCONTROPIN,OUTPUT);
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
  delay(100); 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }
  
  display.display();
  delay(100); 
  display.clearDisplay();
  display.setTextSize(2); 
  display.setTextColor(WHITE);
  display.setCursor(0, 0); 
  display.print("Started");
  display.display();
  // setup temp sensor
  sensors.begin(); 
  oneWire.reset_search();
  oneWire.search(insideThermometer);
  sensors.setResolution(insideThermometer, 12);
  #ifdef DEBUG
    Serial.println("Started");
    Serial.print("input pins: ");
    for(uint8_t i=0;i<inputnum;i++)
    {
      Serial.print(inputpins[i]);
      Serial.print("/");
    }
    Serial.println("");
    // ds18b20
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");
    Serial.print("Parasite power is: "); 
    if (sensors.isParasitePowerMode()) Serial.println("ON");
    else Serial.println("OFF");
    Serial.print("Device 0 Address: ");
    for (uint8_t i = 0; i < 8; i++)
    {
      if (insideThermometer[i] < 16) Serial.print("0");
      Serial.print(insideThermometer[i], HEX);
    }
    Serial.println();
    sensors.requestTemperatures();
    float tempC = sensors.getTempC(insideThermometer);
    Serial.print("Temp C: ");
    Serial.print(tempC);
    Serial.print("Device 0 Resolution: ");
    Serial.print(sensors.getResolution(insideThermometer), DEC); 
    Serial.println();
  #endif
  //target_temperature=20.0;
  union Float {
    float    m_float;
    uint16_t  m_sh[sizeof(float)/2];
  } target_temperature;
  target_temperature.m_float=20.0;
  modbusregisters[TARGET_TEMP_REGISTER_HIGH]=target_temperature.m_sh[1];
  modbusregisters[TARGET_TEMP_REGISTER_LOW]=target_temperature.m_sh[0];
  ReadTemperaureSensor();
  update_display=true;
}





// called after a poll
void polled() {
  DoWriteOutputRegister();
  DoResetInputRegister();
  DoProcessOtherRegisters();
  led.Blink(50,50).Repeat(1);;
}

void DoModBus() {
  int8_t state;
  state = slave.poll( modbusregisters, 8 );
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

void DoProcessOtherRegisters() {
  static float f;
  ACMode=(bool)modbusregisters[ACMode];
  if(f!=(float)*(modbusregisters+TARGET_TEMP_REGISTER_HIGH))
  {
    f=(float)*(modbusregisters+TARGET_TEMP_REGISTER_HIGH);
    update_display=true;
  }
}

void TurnOnHeater() {
  digitalWrite(HEATHERCONTROPIN, HIGH);
}
void TurnOffHeater() {
  digitalWrite(HEATHERCONTROPIN, LOW);
}
void DoThermostat() {
  union Float {
    float    m_float;
    uint16_t  m_sh[sizeof(float)/2];
  } t,s;
  t.m_sh[0]=modbusregisters[CURRENT_TEMP_REGISTER_LOW];
  t.m_sh[1]=modbusregisters[CURRENT_TEMP_REGISTER_HIGH];
  s.m_sh[0]=modbusregisters[TARGET_TEMP_REGISTER_LOW];
  s.m_sh[1]=modbusregisters[TARGET_TEMP_REGISTER_HIGH];
  if(t.m_float<s.m_float) TurnOnHeater(); else TurnOffHeater();
}



void loop() {
  ReadTemperaureSensor();
  DoReadInputs();
  DoLightControl();
  DoModBus();
  DoThermostat();
  DoUpdateDisplay(); // if necessary
  led.Update();
}

void DoUpdateDisplay() {
  if(!update_display) return;

  union Float {
    float    m_float;
    uint16_t  m_sh[sizeof(float)/2];
  } t,s;
  t.m_sh[0]=modbusregisters[CURRENT_TEMP_REGISTER_LOW];
  t.m_sh[1]=modbusregisters[CURRENT_TEMP_REGISTER_HIGH];
  s.m_sh[0]=modbusregisters[TARGET_TEMP_REGISTER_LOW];
  s.m_sh[1]=modbusregisters[TARGET_TEMP_REGISTER_HIGH];

  display.clearDisplay();
  display.setCursor(0, 0); 
  display.print("Temp: ");
  display.println(t.m_float,1);
  display.print("Setp: ");
  display.print(s.m_float,1);
  display.display();

}

void DoLightControl() {
  for(uint8_t i=0;i<lightnum;i++)
    if(lightcontrolbutton[i]->wasPressed()) {
      bitWrite(modbusregisters[BINARY_OUTPUTS_LIGHTS],i,!bitRead(modbusregisters[BINARY_OUTPUTS_LIGHTS],i));
      //Serial.println(modbusregisters[BINARY_OUTPUTS_LIGHTS]);
      DoWriteOutputRegister();
    }
}

void ReadTemperaureSensor() {

  union Float {
    float    m_float;
    uint16_t  m_sh[sizeof(float)/2];
  } current_temperature;
  sensors.requestTemperatures();
  current_temperature.m_float = sensors.getTempC(insideThermometer);
  modbusregisters[CURRENT_TEMP_REGISTER_HIGH]=current_temperature.m_sh[1];
  modbusregisters[CURRENT_TEMP_REGISTER_LOW]=current_temperature.m_sh[0];
  //memcpy(modbusregisters+CURRENT_TEMP_REGISTER_HIGH,&current_temperature, 4);
}