/**
 * @file config.h
 * @brief Master Configuration Parameters and Feature Flags for GotchiLab_.
 * @author José Escobedo Vázquez / MediaLab_
 * @license MIT
 */

#ifndef GOTCHILAB_CONFIG_H
#define GOTCHILAB_CONFIG_H

#include <stdint.h>
#include "pins_config.h"

/* ========================================================================== */
/* FEATURE FLAGS (COMPILE-TIME MODULE INCLUSION)                              */
/* ========================================================================== */

#ifndef USE_BUTTON_SENSOR
#define USE_BUTTON_SENSOR       1   ///< 1: Feed button enabled; 0: Disabled (no starvation)
#endif

#ifndef USE_TOUCH_SENSOR
#define USE_TOUCH_SENSOR        1   ///< 1: Capacitive touch enabled; 0: Auto-hatch & no petting
#endif

#ifndef USE_LIGHT_SENSOR
#define USE_LIGHT_SENSOR        1   ///< 1: LDR sleep/wake enabled; 0: Always awake (no exhaustion)
#endif

#ifndef USE_CO2_SENSOR
#define USE_CO2_SENSOR          1   ///< 1: SCD30 sensor driver compiled; 0: Complete compile-time exclusion
#endif

#ifndef USE_BUZZER
#define USE_BUZZER              1   ///< 1: PWM audio generator active; 0: Total hardware silence
#endif

#ifndef AUTO_HATCH_IF_NO_TOUCH
#define AUTO_HATCH_IF_NO_TOUCH  1   ///< 1: Egg hatches after timer if touch sensor is disabled
#endif

#define AUTO_HATCH_DELAY_MS     3000UL

#ifndef SHOW_STATS_OVERLAY
#define SHOW_STATS_OVERLAY      0   ///< 0: Immersive full-screen animation; 1: Top HUD overlay visible
#endif

/* ========================================================================== */
/* OLED DISPLAY CONFIGURATION (SSD1306)                                       */
/* ========================================================================== */

#define SCREEN_WIDTH            128
#define SCREEN_HEIGHT           64
#define OLED_RESET              -1
#define OLED_ADDRESS            0x3C

/* ========================================================================== */
/* LDR ANALOG SENSOR & SLEEP SYSTEM                                           */
/* ========================================================================== */

/**
 * @brief LDR Darkness Threshold (ESP32 ADC1 12-bit counts: 0 to 4095).
 *
 * Circuit Topology:
 *   VCC (3.3V) ---> [ LDR ] ---> V_out (PIN_LDR / GPIO34) ---> [ R_pull 10kΩ ] ---> GND
 *
 * Voltage Divider Transfer Function:
 *   V_out = VCC * (R_pull / (R_LDR + R_pull))
 *   ADC Counts = (V_out / 3.3V) * 4095
 *
 * Dynamic Response:
 *   - Ambient Room Light: R_LDR is low (~1kΩ - 5kΩ), V_out > 2.3V (ADC counts > ~2856).
 *   - Darkness / Covered: R_LDR increases sharply (> 20kΩ - 100kΩ+), V_out drops below 2.3V.
 *   - Condition `ADC < LDR_DARK_THRESHOLD` flags darkness, transitioning the creature to SLEEP.
 *
 * Multimeter Calibration Note:
 *   - 2856 counts correspond to ~2.30 V on GPIO 34.
 *   - Measure the DC voltage between GPIO 34 and GND in ambient light vs. covered state
 *     to fine-tune this value for your specific sensor batch and workshop environment.
 */
#ifndef LDR_DARK_THRESHOLD
#define LDR_DARK_THRESHOLD      2856
#endif

#define LIGHT_THRESHOLD         LDR_DARK_THRESHOLD  ///< Backward compatibility alias

/**
 * @brief LDR Debugging Telemetry Macro.
 * Set to 1 to periodically dump raw ADC counts, computed input voltage, and dark status
 * to the Serial monitor at 115200 baud to facilitate multimeter calibration.
 */
#ifndef DEBUG_LDR_CALIBRATION
#define DEBUG_LDR_CALIBRATION   0
#endif

#define LDR_DEBUG_INTERVAL_MS   1000UL
#define LIGHT_READ_INTERVAL_MS  500UL

#define MAX_TIME_AWAKE_MS       150000UL ///< Max continuous awake time before exhaustion death (2.5 min)
#define SLEEP_RECOVERY_MULTIPLIER 3      ///< Sleep recovers fatigue 3x faster than wakefulness drains it

/* ========================================================================== */
/* CO2 SENSOR DYNAMICS (SENSIRION SCD30)                                      */
/* ========================================================================== */

#define CO2_READ_INTERVAL_MS    2000UL   ///< SCD30 optical NDIR measurement interval
#define CO2_HIGH_ON_PPM         1600     ///< PPM threshold entering unhealthy / sick state
#define CO2_HIGH_OFF_PPM        1100     ///< PPM threshold recovering to healthy state (hysteresis)

#define MAX_CO2_EXPOSURE_MS     60000UL  ///< Max continuous contaminated air exposure before death (60s)
#define CO2_CLEAN_RECOVERY_MULTIPLIER 3  ///< Health recovery rate multiplier in clean air
#define CO2_COUGH_INTERVAL_MS   12000UL  ///< Sick cough sound period while air is contaminated

/* ========================================================================== */
/* BUTTON, FEEDING & SPAM EXPLOSION (POP)                                     */
/* ========================================================================== */

#define DEBOUNCE_MS             180UL
#define FEED_SPAM_TRIGGER       5        ///< Presses within window that trigger overfeeding explosion
#define FEED_SPAM_WINDOW_MS     2200UL   ///< Time window to detect rapid feeding spam
#define NEWBORN_GRACE_PERIOD_MS 5000UL   ///< Immunity window post-hatch against accidental pop

#define STARVATION_TIME_MS      180000UL ///< Maximum time without food before starvation death (3 min)
#define HUNGER_ALERT_TIME_MS    60000UL  ///< Time without food before stomach starts growling (1 min)
#define HUNGER_SOUND_INTERVAL_MS 20000UL ///< Period of stomach growl alert

/* ========================================================================== */
/* FATIGUE & SLEEP SOUND ALERTS                                               */
/* ========================================================================== */

#define SLEEP_ALERT_TIME_MS     70000UL  ///< Continuous wakefulness time triggering sleepy yawn melody
#define SLEEP_SOUND_INTERVAL_MS 25000UL  ///< Repetition period for sleepy melody

/* ========================================================================== */
/* HAPPINESS & AFFECTION                                                      */
/* ========================================================================== */

#define HAPPINESS_DECAY_MS      15000UL  ///< Every 15s without petting loses 1 point
#define MAX_HAPPINESS           5
#define MIN_HAPPINESS           0

/* ========================================================================== */
/* BUZZER PWM (LEDC) & MUTE CONFIGURATION                                     */
/* ========================================================================== */

#define BUZZER_CHANNEL          0
#define BUZZER_RESOLUTION       8
#define BUZZER_VOLUME           40
#define MUTE_DEBOUNCE_MS        200UL

/* ========================================================================== */
/* GRAPHIC ENGINE & FRAME TIMINGS                                             */
/* ========================================================================== */

#define FRAME_COUNT             15       ///< Number of frames per animation sequence
#define FRAME_SIZE              1024     ///< 128 x 64 pixels / 8 bits = 1024 bytes per frame
#define FRAME_TIME_MS           200UL    ///< 5 frames per second (200 ms per frame)
#define SLEEP_HOLD_FRAME        10       ///< Frame index where sleep animation pauses while dark
#define DEATH_SCREEN_DURATION_MS 9000UL  ///< Final obituary display duration before auto-restart

#endif /* GOTCHILAB_CONFIG_H */
