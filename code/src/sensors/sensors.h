/**
 * @file sensors.h
 * @brief Sensor Abstraction Layer for GotchiLab_.
 * @author José Escobedo Vázquez / MediaLab_
 * @license MIT
 *
 * Provides non-blocking hardware drivers for:
 * - Sensirion SCD30 I2C NDIR CO2 sensor with validation and fair mode bypass.
 * - Push button debouncing and state acquisition.
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the sensor hardware layer.
 * 
 * Configures the push button pin and optionally probes the Sensirion SCD30
 * NDIR CO2 sensor over the shared I2C bus. If fairModeActive is true,
 * SCD30 initialization is omitted completely to protect demonstrations.
 *
 * @param fairModeActive If true, activates CO2 bypass (fair/demo mode).
 */
void initSensors(bool fairModeActive);

/**
 * @brief Polls available sensors in a non-blocking fashion.
 *
 * Checks if new measurement data is ready on the SCD30 and validates
 * reading plausibility (350 ppm to 10000 ppm range filtering).
 */
void updateSensors(void);

/**
 * @brief Retrieves the latest validated CO2 concentration.
 *
 * @return Current CO2 concentration in parts per million (ppm).
 *         Returns 400 ppm (nominal atmospheric baseline) if the sensor is disabled.
 */
int getCO2(void);

/**
 * @brief Queries whether the push button is currently depressed.
 *
 * @return true if button input is active LOW (pressed), false otherwise.
 */
bool isButtonPressed(void);

/**
 * @brief Checks if the SCD30 sensor was successfully detected on I2C.
 *
 * @return true if physically detected and functioning, false otherwise.
 */
bool isCO2Connected(void);

/**
 * @brief Checks if the CO2 sensor is active and not disabled by fair mode jumper.
 *
 * @return true if enabled and operative, false if bypassed or disabled.
 */
bool isCO2SensorEnabled(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSORS_H */