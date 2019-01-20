# ArduinoModBusHomeAssistant
Arduino sketch for modbus implementation of sensor, switch and thermostat to use with Home Assistant.
This code make the arduino (I've used an a 5V nano) act a Modbus device than  can be connected to a Home Assistant controller. Connectione can be made via a usb cable or via 485 bus. Only in this mode you can connect more than a device and with long cables (more than 1000m).
Home Assistant deals with 4 types of device:
- binary sensors (digital readonly inputs)
- Temperature sensors
- Switch (or coils in Modbus language: digital output that can also be read)
- Climate (a block that act as a thermostat)

This arduino can be 1 or all of these object: it can be a temperature sensor if a sensor is connected. Can be a Climate (sensor also needed). Can read up to 16 inputs and control up to 16 outputs (this arduino have less this number of pins).
The temperature read by sensor is mapped in the current_temp_register that can be read by HA.
Inputs and outputs are mapped to binary_inputs register and binary_outputs register respectively.

See electical schematic for details.

This hardware can serve to many goals:
it can be a headless remote actuator (a switch) to control a load (a lihgt, a motor, etc)
it can be a headless remote sensor for temperature reading of to check magnetic sensors
it can have a display and buttons to show temperature or control lighs or other devices

Modbus registers map
All registers are 16 bits wide

reg#    reg name                    function
0       target_temp_register_low    setpoint temperature
1       target_temp_register_high
2       current_temp_register_low   current tempperature read by sensor
3       current_temp_register_high
4       BINARY_INPUTS               read the inputs connected (read only)
5       BINARY_OUTPUTS              read and set the outputs connected