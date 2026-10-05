/**
 * @file pins_config.h
 * @brief Centralized GPIO Pinout Configuration for GotchiLab_.
 * @author José Escobedo Vázquez / MediaLab_
 * @license MIT
 */

#ifndef GOTCHILAB_PINS_CONFIG_H
#define GOTCHILAB_PINS_CONFIG_H

#include <stdint.h>

/* ========================================================================== */
/* HARDWARE PIN ASSIGNMENTS                                                   */
/* ========================================================================== */

/**
 * @name I2C Bus Configuration (SSD1306 OLED & SCD30 CO2 Sensor)
 * Shared I2C bus on ESP32 DevKit v1: SDA en GPIO 22, SCL en GPIO 21.
 */
///@{
#define PIN_I2C_SDA         22  ///< I2C Data line (Wire)
#define PIN_I2C_SCL         21  ///< I2C Clock line (Wire)
///@}

/**
 * @name User Interaction Inputs
 */
///@{
#define PIN_BUTTON          33  ///< Push button for feeding (INPUT_PULLUP, active LOW)
#define PIN_TOUCH           27  ///< TTP223 capacitive touch sensor (INPUT, active HIGH)
///@}

/**
 * @name Environmental Sensors
 */
///@{
#define PIN_LDR             34  ///< Analog LDR light sensor (ADC1_CH6, input only)
///@}

/**
 * @name Audio Actuators and Controls
 */
///@{
#define PIN_BUZZER          25  ///< Passive piezo buzzer driven via LEDC PWM
#define PIN_MUTE            32  ///< Audio mute toggle jumper/switch (INPUT_PULLUP, active LOW)
///@}

/**
 * @name Hardware Bypass Jumpers
 */
///@{
#define PIN_FAIR_MODE       26  ///< Fair Mode / CO2 bypass jumper (INPUT_PULLUP, active LOW)
#define PIN_CO2_DISABLE     PIN_FAIR_MODE
///@}

/* ========================================================================== */
/* BACKWARD COMPATIBILITY ALIASES                                             */
/* ========================================================================== */

#define OLED_SDA            PIN_I2C_SDA
#define OLED_SCL            PIN_I2C_SCL
#define BUTTON_PIN          PIN_BUTTON
#define TOUCH_PIN           PIN_TOUCH
#define LIGHT_SENSOR_PIN    PIN_LDR
#define BUZZER_PIN          PIN_BUZZER
#define MUTE_PIN            PIN_MUTE
#define FAIR_MODE_PIN       PIN_FAIR_MODE

#endif /* GOTCHILAB_PINS_CONFIG_H */
