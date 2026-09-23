#ifndef SENSORS_H
#define SENSORS_H

void initSensors();
void updateSensors();

int getCO2();
bool isButtonPressed();
bool isCO2Connected();

#endif