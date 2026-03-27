#include "sensors.h"
#include "../config/config.h"

#if USE_CO2_SENSOR
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
SCD30 scd30;
#endif

int co2Value = 400;

void initSensors() {

#if USE_CO2_SENSOR
    if (scd30.begin() == false) {
        Serial.println("[SENSORS] SCD30 no detectado");
    } else {
        Serial.println("[SENSORS] SCD30 iniciado");
    }
#endif

#if USE_BUTTON_SENSOR
    pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
}

void updateSensors() {

#if USE_CO2_SENSOR
    if (scd30.dataAvailable()) {
        scd30.readMeasurement();
        co2Value = scd30.getCO2();
    }
#endif

}

int getCO2() {
    return co2Value;
}

bool isButtonPressed() {
#if USE_BUTTON_SENSOR
    return digitalRead(BUTTON_PIN) == LOW;
#else
    return false;
#endif
}