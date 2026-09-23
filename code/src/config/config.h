#ifndef GOTCHILAB_CONFIG_H
#define GOTCHILAB_CONFIG_H

// =====================================================
// ACTIVAR / DESACTIVAR MODULOS
// =====================================================

#define USE_BUTTON_SENSOR       1
#define USE_TOUCH_SENSOR        1
#define USE_LIGHT_SENSOR        1
#define USE_CO2_SENSOR          1
#define USE_BUZZER              1

// Si NO hay sensor touch, el huevo nace solo
#define AUTO_HATCH_IF_NO_TOUCH  1
#define AUTO_HATCH_DELAY_MS     3000

// Mostrar barra superior de estadísticas (0: oculta para modo inmersivo donde debes cuidarlo a ciegas, 1: visible)
#define SHOW_STATS_OVERLAY      0

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

// Pin para mutear/desmutear: Al puentearlo a GND alterna entre silencio y sonido
#define MUTE_PIN 27
#define MUTE_DEBOUNCE_MS 200

// =====================================================
// LUZ & SUEÑO / AGOTAMIENTO
// =====================================================

#define LIGHT_THRESHOLD 1500
#define LIGHT_READ_INTERVAL_MS 500

// Máximo tiempo sin dormir (si el sensor de luz está activo) -> Muerte por agotamiento: 2.5 min
#define MAX_TIME_AWAKE_MS       150000UL
// Multiplicador de recuperación de sueño progresivo (recupera energía el triple de rápido que el gasto en vigilia)
#define SLEEP_RECOVERY_MULTIPLIER 3

// =====================================================
// CO2 (SCD30)
// =====================================================

#define CO2_READ_INTERVAL_MS 2000

#define CO2_HIGH_ON_PPM   1600
#define CO2_HIGH_OFF_PPM  1100

// Tiempo continuo acumulado con CO2 alto para morir por asfixia/intoxicación: 60 segundos
#define MAX_CO2_EXPOSURE_MS     60000UL
// Factor de recuperación rápida al volver el aire limpio (se recupera 3x más rápido)
#define CO2_CLEAN_RECOVERY_MULTIPLIER 3

// =====================================================
// BOTON / ALIMENTACION / SOBREALIMENTACION
// =====================================================

#define DEBOUNCE_MS 180

#define FEED_SPAM_TRIGGER 5
#define FEED_SPAM_WINDOW_MS 2200

// Tiempo sin comer para morir de inanición: 3 minutos (180,000 ms)
#define STARVATION_TIME_MS      180000UL

// =====================================================
// FELICIDAD / CARICIAS
// =====================================================

// Intervalo de decaimiento de felicidad: cada 15 segundos pierde 1 punto
#define HAPPINESS_DECAY_MS      15000UL
#define MAX_HAPPINESS           5
#define MIN_HAPPINESS           0

// =====================================================
// ANIMACION Y FRAMES
// =====================================================

#define FRAME_COUNT 15
#define FRAME_SIZE 1024
#define FRAME_TIME_MS 200

#define SLEEP_HOLD_FRAME 10

// Duración de la pantalla de Game Over antes de reiniciar a IDLE_EGG (en ms)
#define DEATH_SCREEN_DURATION_MS 9000UL

#endif