# Sensirion
Simple Arduino logger for a Sensirion SPS30 particulate matter sensor

Hardware:
* Arduino Uno
* SD + RTC shield (Adafruit 1141 or equivalent)
* Sensirion SPS30

The Sensirion is attached to the Arduino's UART. Note that the D0/RX pin is shared with the serial programmer circuitry -- I found it necessary to detach that pin while programming.
