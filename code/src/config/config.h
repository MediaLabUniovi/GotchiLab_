#ifndef GOTCHILAB_CONFIG_H
#define GOTCHILAB_CONFIG_H

// =====================================================
// ACTIVAR / DESACTIVAR MODULOS
// =====================================================

#define USE_BUTTON_SENSOR       1
#define USE_TOUCH_SENSOR        0
#define USE_LIGHT_SENSOR        1
#define USE_CO2_SENSOR          1
#define USE_BUZZER              1

// Si NO hay sensor touch, el huevo nace solo
#define AUTO_HATCH_IF_NO_TOUCH  1
#define AUTO_HATCH_DELAY_MS     3000

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

#define OLED_SDA 22
#define OLED_SCL 21

// =====================================================
// PINES
// =====================================================

#define BUTTON_PIN 33
#define TOUCH_PIN  14   // <-- TTP223

#define LIGHT_SENSOR_PIN 34

#define BUZZER_PIN 26
#define BUZZER_CHANNEL 0
#define BUZZER_RESOLUTION 8
#define BUZZER_VOLUME 40

// =====================================================
// LUZ
// =====================================================

#define LIGHT_THRESHOLD 1500
#define LIGHT_READ_INTERVAL_MS 500

// =====================================================
// CO2 (SCD30)
// =====================================================

#define CO2_READ_INTERVAL_MS 2000

#define CO2_HIGH_ON_PPM   1400
#define CO2_HIGH_OFF_PPM  1000

// =====================================================
// BOTON / ANIMACION
// =====================================================

#define DEBOUNCE_MS 180

#define FEED_SPAM_TRIGGER 5
#define FEED_SPAM_WINDOW_MS 2200

#define FRAME_COUNT 15
#define FRAME_SIZE 1024
#define FRAME_TIME_MS 200

#define SLEEP_HOLD_FRAME 10

#endif