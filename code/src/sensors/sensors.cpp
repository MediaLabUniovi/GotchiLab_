/**
 * @file sensors.cpp
 * @brief Implementation of Sensor Abstraction Layer for GotchiLab_.
 * @author José Escobedo Vázquez / MediaLab_
 * @license MIT
 */

#include "sensors.h"
#include <Arduino.h>
#include "config/config.h"

#if USE_CO2_SENSOR
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
static SCD30 scd30;
static bool scd30Available = false;
#endif

static bool co2SensorEnabled = false;
static int co2Value = 400;

void initSensors(bool fairModeActive)
{
#if USE_BUTTON_SENSOR
    pinMode(PIN_BUTTON, INPUT_PULLUP);
#endif

#if USE_CO2_SENSOR
    if (fairModeActive) {
        Serial.println(F("[SENSORS] Jumper de MODO FERIA detectado (GND)."));
        Serial.println(F("[SENSORS] Bypass de CO2 activado: Sensor deshabilitado, operando en linea base limpia (400 ppm)."));
        co2SensorEnabled = false;
        scd30Available = false;
        co2Value = 400;
        return;
    }

    co2SensorEnabled = true;

    // Iniciar SCD30 con auto-calibración deshabilitada por defecto para evitar distorsiones
    if (scd30.begin() == false) {
        Serial.println(F("[SENSORS] SCD30 no detectado en bus I2C (Verifique conexion SDA/SCL)"));
        scd30Available = false;
    } else {
        Serial.println(F("[SENSORS] SCD30 NDIR iniciado correctamente."));
        scd30Available = true;
        scd30.setMeasurementInterval(2);
        scd30.setAutoSelfCalibration(false);
    }
#else
    (void)fairModeActive;
    co2SensorEnabled = false;
#endif
}

void updateSensors(void)
{
#if USE_CO2_SENSOR
    if (co2SensorEnabled && scd30Available && scd30.dataAvailable()) {
        scd30.readMeasurement();
        uint16_t rawCO2 = scd30.getCO2();

        // Validacion de rango fisico plausible (SCD30: 350 - 10000 ppm)
        if (rawCO2 >= 350 && rawCO2 <= 10000) {
            co2Value = static_cast<int>(rawCO2);
        } else {
            Serial.print(F("[SENSORS] Lectura SCD30 anomala descartada: "));
            Serial.println(rawCO2);
        }
    }
#endif
}

int getCO2(void)
{
    return co2Value;
}

bool isCO2Connected(void)
{
#if USE_CO2_SENSOR
    return (scd30Available && co2SensorEnabled);
#else
    return false;
#endif
}

bool isCO2SensorEnabled(void)
{
    return co2SensorEnabled;
}

bool isButtonPressed(void)
{
#if USE_BUTTON_SENSOR
    return digitalRead(PIN_BUTTON) == LOW;
#else
    return false;
#endif
}