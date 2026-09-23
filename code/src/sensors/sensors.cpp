#include "sensors.h"
#include "../config/config.h"

#if USE_CO2_SENSOR
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
SCD30 scd30;
bool scd30Available = false;
#endif

int co2Value = 400;

void initSensors() {

#if USE_CO2_SENSOR
    // Iniciar SCD30 con auto-calibración deshabilitada por defecto para evitar lecturas distorsionadas
    if (scd30.begin() == false) {
        Serial.println("[SENSORS] SCD30 no detectado en bus I2C (Verifique cables SDA/SCL)");
        scd30Available = false;
    } else {
        Serial.println("[SENSORS] SCD30 iniciado correctamente");
        scd30Available = true;
        // Intervalo de lectura de 2 segundos en el sensor
        scd30.setMeasurementInterval(2);
        // Si el usuario sopla o el sensor se descalibró, el ASC puede disparar el offset
        scd30.setAutoSelfCalibration(false); 
    }
#endif

#if USE_BUTTON_SENSOR
    pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
}

void updateSensors() {

#if USE_CO2_SENSOR
    if (scd30Available && scd30.dataAvailable()) {
        scd30.readMeasurement();
        uint16_t rawCO2 = scd30.getCO2();
        
        // Validación de rango físico plausible (NDIR SCD30 mide entre 400 y 10000 ppm)
        if (rawCO2 >= 350 && rawCO2 <= 10000) {
            co2Value = rawCO2;
        } else {
            Serial.print("[SENSORS] Lectura SCD30 anómala o descartada: ");
            Serial.println(rawCO2);
        }
    }
#endif

}

int getCO2() {
    return co2Value;
}

bool isCO2Connected() {
#if USE_CO2_SENSOR
    return scd30Available;
#else
    return false;
#endif
}

bool isButtonPressed() {
#if USE_BUTTON_SENSOR
    return digitalRead(BUTTON_PIN) == LOW;
#else
    return false;
#endif
}