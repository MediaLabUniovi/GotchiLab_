/**
 * @file main.cpp
 * @brief GotchiLab_ - Firmware Principal de Mascota Virtual Educativa STEAM.
 * @author José Escobedo Vázquez / MediaLab_
 * @license MIT
 *
 * Arquitectura modular sobre ESP32 DevKit v1 con display OLED SSD1306,
 * sensor NDIR Sensirion SCD30, fotorresistencia LDR calibrada y actuador piezoeléctrico.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config/config.h"
#include "sensors/sensors.h"

extern "C" {
    #include "animations/penguin_feed_anim.h"
    #include "animations/penguin_idle_anim.h"
    #include "animations/penguin_pet_anim.h"
    #include "animations/penguin_sleep_anim.h"
    #include "animations/penguin_pop_anim.h"
    #include "animations/penguin_idle_egg_anim.h"
    #include "animations/penguin_birth_anim.h"
    #include "animations/penguin_idle_unhealthy_anim.h"
    #include "animations/penguin_transition_unhealthy_anim.h"
}

/* ========================================================================== */
/* HARDWARE INSTANCES & BITMAPS                                               */
/* ========================================================================== */

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/// Icono bitmap 1-bit de muslo de comida para HUD (5x5 px)
static const unsigned char PROGMEM chicken_leg_icon[] = {
    0b01110000,
    0b11111000,
    0b11110000,
    0b01100000,
    0b00110000
};

/* ========================================================================== */
/* FSM & LIFE CYCLE ENUMS                                                     */
/* ========================================================================== */

/**
 * @brief Estados principales de la Máquina de Estados Finita (FSM).
 */
enum AnimationState {
    FEED = 0,                    ///< Comiendo alimento (bloqueante)
    IDLE = 1,                    ///< Reposo en estado saludable
    PET = 2,                     ///< Recibiendo caricia táctil (bloqueante)
    SLEEP = 3,                   ///< Durmiendo en penumbra
    POP = 4,                     ///< Animación de explosión/desvanecimiento post-mortem (bloqueante)
    IDLE_EGG = 5,                ///< Huevo esperando eclosión
    BIRTH = 6,                   ///< Eclosión y nacimiento (bloqueante)
    IDLE_UNHEALTHY = 7,          ///< Reposo enfermo por atmósfera viciada (CO2 alto)
    TRANSITION_TO_UNHEALTHY = 8, ///< Transición gráfica hacia estado enfermo
    TRANSITION_TO_HEALTHY = 9,   ///< Transición de recuperación hacia estado sano
    DEAD = 10                    ///< Pantalla final de defunción y Game Over
};

/**
 * @brief Razones clínicas de fallecimiento de la criatura.
 */
enum DeathReason {
    DEATH_NONE = 0,
    DEATH_STARVATION,            ///< Inanición por falta de alimento (> 3 minutos)
    DEATH_OVERFED,               ///< Sobrealimentación crítica por spam de pulsaciones (POP)
    DEATH_CO2,                   ///< Asfixia por exposición continua a CO2 (> 60 segundos)
    DEATH_EXHAUSTION             ///< Agotamiento extremo por falta de descanso (> 2.5 minutos)
};

static AnimationState currentAnimation = IDLE_EGG;
static uint8_t currentFrame = 0;

static DeathReason pendingDeathReason = DEATH_NONE;
static DeathReason deathReason = DEATH_NONE;
static uint32_t deathTime = 0;

/* ========================================================================== */
/* AUDIO SYSTEM & TONE TABLES                                                 */
/* ========================================================================== */

struct Note {
    uint16_t freq;
    uint16_t duration;
};

static const Note SOUND_FEED[] = {
    {523, 70}, {659, 70}, {784, 90}, {1047, 120}, {0, 30}
};

static const Note SOUND_PET[] = {
    {659, 60}, {784, 60}, {659, 80}, {0, 25}
};

static const Note SOUND_IDLE[] = {
    {523, 40}, {659, 50}, {0, 20}
};

static const Note SOUND_SLEEPY[] = {
    {784, 130}, {659, 150}, {587, 170}, {523, 200},
    {0, 60},
    {440, 200}, {392, 230}, {330, 280}, {262, 400}, {0, 80}
};

static const Note SOUND_SLEEP[] = {
    {784, 130}, {659, 150}, {587, 170}, {523, 200},
    {0, 60},
    {440, 200}, {392, 230}, {330, 280}, {262, 400}, {0, 80}
};

static const Note SOUND_HUNGER_GROWL[] = {
    {98, 90}, {115, 80}, {82, 100}, {123, 70}, {75, 120},
    {0, 40},
    {87, 100}, {110, 80}, {92, 90}, {73, 140}, {0, 60}
};

static const Note SOUND_COUGH[] = {
    {370, 35}, {240, 45}, {140, 60}, {0, 70},
    {390, 35}, {260, 45}, {150, 70}, {0, 90},
    {310, 40}, {200, 50}, {120, 80}, {0, 60}
};

static const Note SOUND_EGG[] = {
    {392, 70}, {440, 70}, {392, 80}, {0, 40}
};

static const Note SOUND_BIRTH[] = {
    {523, 60}, {659, 60}, {784, 60}, {988, 80}, {1175, 110}, {0, 40}
};

static const Note SOUND_POP[] = {
    {220, 40}, {180, 35}, {140, 35}, {90, 70}, {0, 30}
};

static const Note SOUND_UNHEALTHY[] = {
    {370, 35}, {240, 45}, {140, 60}, {0, 70},
    {390, 35}, {260, 45}, {150, 70}, {0, 60}
};

static const Note SOUND_TRANSITION_UNHEALTHY[] = {
    {587, 60}, {523, 70}, {440, 80}, {0, 30},
    {370, 40}, {240, 50}, {140, 70}, {0, 40}
};

static const Note SOUND_TRANSITION_RECOVER[] = {
    {440, 60}, {523, 70}, {659, 90}, {0, 30}
};

static const Note SOUND_DEATH[] = {
    {440, 300}, {440, 300}, {440, 300}, {349, 250}, {523, 100},
    {440, 300}, {349, 250}, {523, 100}, {440, 600}, {0, 100}
};

static const Note* currentSound = nullptr;
static uint8_t currentSoundLength = 0;
static uint8_t currentSoundIndex = 0;
static bool soundPlaying = false;
static uint32_t soundLastChange = 0;

/* ========================================================================== */
/* STATE VARIABLES & TELEMETRY TIMERS                                         */
/* ========================================================================== */

static uint32_t lastButtonTime = 0;
static bool lastButtonPhysicalState = HIGH;

static uint32_t lastLightReadTime = 0;
static bool isDark = false;
static bool lastDarkState = false;

static uint32_t lastTouchReadTime = 0;
static bool lastTouchDetected = false;
static bool touchSensorInitialized = false;

static uint32_t lastCO2LogicTime = 0;
static uint16_t currentCO2ppm = 400;
static uint16_t displaySmoothCO2 = 400;
static bool co2High = false;

// Tiempos vitales
static uint32_t hatchTime = 0;
static uint32_t lastFeedTestTime = 0;
static uint32_t highCO2AccumulatedMs = 0;
static uint32_t lastCO2CheckTime = 0;
static uint32_t continuousAwakeTime = 0;
static uint32_t lastAwakeCheckTime = 0;
static uint32_t lastHappinessDecayTime = 0;

// Estadísticas de partida
static int8_t currentHappiness = MAX_HAPPINESS;
static uint32_t totalPets = 0;
static uint32_t totalFeeds = 0;

static bool visualUnhealthy = false;
static bool sleepHoldFrame = false;
static bool feedRequested = false;
static bool isHatched = false;
static bool birthTriggered = false;

static uint8_t rapidFeedCount = 0;
static uint32_t firstFeedPressTime = 0;

static uint32_t eggStartTime = 0;
static uint32_t lastFrameTime = 0;

static bool isMuted = false;
static uint32_t lastMuteToggleTime = 0;
static bool lastMutePhysicalState = HIGH;

// Alertas sonoras periódicas
static uint32_t lastHungerSoundTime = 0;
static uint32_t lastSleepySoundTime = 0;
static uint32_t lastCoughSoundTime = 0;

/* ========================================================================== */
/* FUNCTION PROTOTYPES                                                        */
/* ========================================================================== */

const char* getAnimationName(AnimationState anim);
void playAnimationSound(AnimationState anim);
bool hasLightFeature(void);
bool hasCO2Feature(void);
bool hasButtonFeature(void);
bool hasTouchFeature(void);
bool isTransitionAnimation(AnimationState anim);
bool isBusyAnimation(AnimationState anim);

void drawStatsOverlay(void);
void drawFrameRaw(const uint8_t* frameData, int yOffset);
void getCurrentAnimationFrame(const uint8_t** frameData, int* yOffset);
void refreshCurrentFrame(void);
void setAnimation(AnimationState newAnim);
AnimationState getCurrentBaseState(void);
bool tryStartHealthTransition(void);
void goToBaseState(void);

void triggerDeath(DeathReason reason);
void drawDeathScreen(void);
void resetToEggState(void);

void playSoundSequence(const Note* sequence, uint8_t length);
void stopSound(void);
void updateSound(void);

void handleButton(void);
void handleLightSensor(void);
void handleTouchSensor(void);
void handleAutoHatch(void);
void handleCO2Sensor(void);
void handleMutePin(void);

void updateGameStats(void);
void checkConditionSounds(void);
bool isTouchActive(void);
void showBootMessage(void);

/* ========================================================================== */
/* AUDIO DRIVER IMPLEMENTATION                                                */
/* ========================================================================== */

void playSoundSequence(const Note* sequence, uint8_t length)
{
#if USE_BUZZER
    if (isMuted) return;

    currentSound = sequence;
    currentSoundLength = length;
    currentSoundIndex = 0;
    soundPlaying = true;
    soundLastChange = 0;
#else
    (void)sequence;
    (void)length;
#endif
}

void stopSound(void)
{
#if USE_BUZZER
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWrite(BUZZER_CHANNEL, 0);
    soundPlaying = false;
    currentSound = nullptr;
    currentSoundLength = 0;
    currentSoundIndex = 0;
#endif
}

void updateSound(void)
{
#if USE_BUZZER
    if (isMuted) {
        if (soundPlaying) stopSound();
        return;
    }

    if (!soundPlaying || currentSound == nullptr) return;

    uint32_t now = millis();

    if (soundLastChange == 0) {
        if (currentSound[currentSoundIndex].freq > 0) {
            ledcWriteTone(BUZZER_CHANNEL, currentSound[currentSoundIndex].freq);
            ledcWrite(BUZZER_CHANNEL, BUZZER_VOLUME);
        } else {
            ledcWriteTone(BUZZER_CHANNEL, 0);
            ledcWrite(BUZZER_CHANNEL, 0);
        }
        soundLastChange = now;
        return;
    }

    if (now - soundLastChange >= currentSound[currentSoundIndex].duration) {
        currentSoundIndex++;

        if (currentSoundIndex >= currentSoundLength) {
            stopSound();
            return;
        }

        if (currentSound[currentSoundIndex].freq > 0) {
            ledcWriteTone(BUZZER_CHANNEL, currentSound[currentSoundIndex].freq);
            ledcWrite(BUZZER_CHANNEL, BUZZER_VOLUME);
        } else {
            ledcWriteTone(BUZZER_CHANNEL, 0);
            ledcWrite(BUZZER_CHANNEL, 0);
        }

        soundLastChange = now;
    }
#endif
}

const char* getAnimationName(AnimationState anim)
{
    switch (anim) {
        case FEED:                    return "FEED";
        case IDLE:                    return "IDLE";
        case PET:                     return "PET";
        case SLEEP:                   return "SLEEP";
        case POP:                     return "POP";
        case IDLE_EGG:                return "IDLE_EGG";
        case BIRTH:                   return "BIRTH";
        case IDLE_UNHEALTHY:          return "IDLE_UNHEALTHY";
        case TRANSITION_TO_UNHEALTHY: return "TRANSITION_TO_UNHEALTHY";
        case TRANSITION_TO_HEALTHY:   return "TRANSITION_TO_HEALTHY";
        case DEAD:                    return "DEAD";
        default:                      return "UNKNOWN";
    }
}

void playAnimationSound(AnimationState anim)
{
    switch (anim) {
        case FEED:
            playSoundSequence(SOUND_FEED, sizeof(SOUND_FEED) / sizeof(SOUND_FEED[0]));
            break;
        case PET:
            playSoundSequence(SOUND_PET, sizeof(SOUND_PET) / sizeof(SOUND_PET[0]));
            break;
        case IDLE:
            playSoundSequence(SOUND_IDLE, sizeof(SOUND_IDLE) / sizeof(SOUND_IDLE[0]));
            break;
        case SLEEP:
            playSoundSequence(SOUND_SLEEP, sizeof(SOUND_SLEEP) / sizeof(SOUND_SLEEP[0]));
            break;
        case IDLE_EGG:
            playSoundSequence(SOUND_EGG, sizeof(SOUND_EGG) / sizeof(SOUND_EGG[0]));
            break;
        case BIRTH:
            playSoundSequence(SOUND_BIRTH, sizeof(SOUND_BIRTH) / sizeof(SOUND_BIRTH[0]));
            break;
        case POP:
            playSoundSequence(SOUND_POP, sizeof(SOUND_POP) / sizeof(SOUND_POP[0]));
            break;
        case IDLE_UNHEALTHY:
            playSoundSequence(SOUND_UNHEALTHY, sizeof(SOUND_UNHEALTHY) / sizeof(SOUND_UNHEALTHY[0]));
            break;
        case TRANSITION_TO_UNHEALTHY:
            playSoundSequence(SOUND_TRANSITION_UNHEALTHY, sizeof(SOUND_TRANSITION_UNHEALTHY) / sizeof(SOUND_TRANSITION_UNHEALTHY[0]));
            break;
        case TRANSITION_TO_HEALTHY:
            playSoundSequence(SOUND_TRANSITION_RECOVER, sizeof(SOUND_TRANSITION_RECOVER) / sizeof(SOUND_TRANSITION_RECOVER[0]));
            break;
        case DEAD:
            playSoundSequence(SOUND_DEATH, sizeof(SOUND_DEATH) / sizeof(SOUND_DEATH[0]));
            break;
    }
}

/* ========================================================================== */
/* LOGIC HELPERS & FEATURE AVAILABILITY                                       */
/* ========================================================================== */

bool hasLightFeature(void)  { return USE_LIGHT_SENSOR; }
bool hasCO2Feature(void)    { return (USE_CO2_SENSOR && isCO2SensorEnabled() && isCO2Connected()); }
bool hasButtonFeature(void) { return USE_BUTTON_SENSOR; }
bool hasTouchFeature(void)  { return USE_TOUCH_SENSOR; }

bool isTransitionAnimation(AnimationState anim)
{
    return (anim == TRANSITION_TO_UNHEALTHY || anim == TRANSITION_TO_HEALTHY);
}

bool isBusyAnimation(AnimationState anim)
{
    return (anim == FEED ||
            anim == PET ||
            anim == SLEEP ||
            anim == POP ||
            anim == IDLE_EGG ||
            anim == BIRTH ||
            anim == TRANSITION_TO_UNHEALTHY ||
            anim == TRANSITION_TO_HEALTHY ||
            anim == DEAD);
}

/* ========================================================================== */
/* GRAPHICS ENGINE & OVERLAY                                                  */
/* ========================================================================== */

void drawStatsOverlay(void)
{
#if !SHOW_STATS_OVERLAY
    return;
#endif

    if (!isHatched || currentAnimation == IDLE_EGG || currentAnimation == BIRTH || 
        currentAnimation == POP || currentAnimation == DEAD) {
        return;
    }

    // 1. Hambre (Muslitos)
    if (hasButtonFeature()) {
        uint32_t elapsed = millis() - lastFeedTestTime;
        int hungerLevel = 3;
        if (elapsed > (STARVATION_TIME_MS * 2 / 3)) {
            hungerLevel = 1;
        } else if (elapsed > (STARVATION_TIME_MS / 3)) {
            hungerLevel = 2;
        }

        for (int i = 0; i < 3; i++) {
            int x = 2 + i * 7;
            int y = 2;
            if (i < hungerLevel) {
                display.drawBitmap(x, y, chicken_leg_icon, 5, 5, SSD1306_WHITE);
            } else {
                display.drawRect(x, y, 5, 5, SSD1306_WHITE);
            }
        }
    }

    // 2. Felicidad (Corazones)
    if (hasTouchFeature()) {
        int xStart = 28;
        int y = 2;
        int filled = (currentHappiness > 0) ? (currentHappiness > 2 ? (currentHappiness >= 4 ? 3 : 2) : 1) : 0;
        for (int i = 0; i < 3; i++) {
            int x = xStart + i * 7;
            if (i < filled) {
                display.drawPixel(x+1, y, SSD1306_WHITE);
                display.drawPixel(x+3, y, SSD1306_WHITE);
                display.drawFastHLine(x, y+1, 5, SSD1306_WHITE);
                display.drawFastHLine(x+1, y+2, 3, SSD1306_WHITE);
                display.drawPixel(x+2, y+3, SSD1306_WHITE);
            } else {
                display.drawPixel(x+1, y, SSD1306_WHITE);
                display.drawPixel(x+3, y, SSD1306_WHITE);
                display.drawPixel(x, y+1, SSD1306_WHITE);
                display.drawPixel(x+4, y+1, SSD1306_WHITE);
                display.drawPixel(x+2, y+3, SSD1306_WHITE);
            }
        }
    }

    // 3. Salud Respiratoria / Aire Limpio (CO2)
    if (hasCO2Feature()) {
        int barX = 54;
        int barY = 2;
        int barW = 34;
        int barH = 5;

        display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);

        if (displaySmoothCO2 < currentCO2ppm) {
            displaySmoothCO2 += (currentCO2ppm - displaySmoothCO2 + 3) / 4;
        } else if (displaySmoothCO2 > currentCO2ppm) {
            uint16_t diff = displaySmoothCO2 - currentCO2ppm;
            uint16_t step = (diff / 2) + 20;
            if (displaySmoothCO2 > currentCO2ppm + step) {
                displaySmoothCO2 -= step;
            } else {
                displaySmoothCO2 = currentCO2ppm;
            }
        }

        int ppm = displaySmoothCO2;
        if (ppm < 400) ppm = 400;
        if (ppm > 2000) ppm = 2000;
        int fillW = map(ppm, 400, 2000, barW - 2, 0);
        if (fillW > 0) {
            display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);
        }
    }

    // 4. Energía / Sueño (LDR)
    if (hasLightFeature()) {
        int zX = 94;
        int zY = 2;

        uint32_t awakeTime = continuousAwakeTime;
        if (awakeTime > MAX_TIME_AWAKE_MS) awakeTime = MAX_TIME_AWAKE_MS;
        int energyRemaining = MAX_TIME_AWAKE_MS - awakeTime;
        int barW = 28;

        display.drawRect(zX, zY, barW, 5, SSD1306_WHITE);
        int fill = map(energyRemaining, 0, MAX_TIME_AWAKE_MS, 0, barW - 2);
        if (fill > 0) {
            display.fillRect(zX + 1, zY + 1, fill, 3, SSD1306_WHITE);
        }

        if (currentAnimation == SLEEP) {
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(zX + 10, zY - 1);
            if ((millis() / 500) % 2 == 0) {
                display.print(F("z"));
            }
        }
    }

    display.drawFastHLine(0, 8, SCREEN_WIDTH, SSD1306_WHITE);
}

void drawFrameRaw(const uint8_t* frameData, int yOffset)
{
    uint8_t* buffer = display.getBuffer();
    memset(buffer, 0x00, FRAME_SIZE);

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            int srcY = y - yOffset;
            if (srcY < 0 || srcY >= SCREEN_HEIGHT) continue;

            int srcIndex = x + (srcY / 8) * SCREEN_WIDTH;
            uint8_t srcBit = 1 << (srcY & 7);

            if (frameData[srcIndex] & srcBit) {
                display.drawPixel(x, y, SSD1306_WHITE);
            }
        }
    }

    drawStatsOverlay();
    display.display();
}

void getCurrentAnimationFrame(const uint8_t** frameData, int* yOffset)
{
    uint8_t frameIndex = currentFrame;

    switch (currentAnimation) {
        case FEED:
            *frameData = penguin_feed_anim[frameIndex];
            *yOffset = penguin_feed_offsets[frameIndex];
            break;
        case IDLE:
            *frameData = penguin_idle_anim[frameIndex];
            *yOffset = penguin_idle_offsets[frameIndex];
            break;
        case PET:
            *frameData = penguin_pet_anim[frameIndex];
            *yOffset = penguin_pet_offsets[frameIndex];
            break;
        case SLEEP:
            *frameData = penguin_sleep_anim[frameIndex];
            *yOffset = penguin_sleep_offsets[frameIndex];
            break;
        case POP:
            *frameData = penguin_pop_anim[frameIndex];
            *yOffset = penguin_pop_offsets[frameIndex];
            break;
        case IDLE_EGG:
            *frameData = penguin_idle_egg_anim[frameIndex];
            *yOffset = penguin_idle_egg_offsets[frameIndex];
            break;
        case BIRTH:
            *frameData = penguin_birth_anim[frameIndex];
            *yOffset = penguin_birth_offsets[frameIndex];
            break;
        case IDLE_UNHEALTHY:
            *frameData = penguin_idle_unhealthy_anim[frameIndex];
            *yOffset = penguin_idle_unhealthy_offsets[frameIndex];
            break;
        case TRANSITION_TO_UNHEALTHY:
            *frameData = penguin_transition_unhealthy_anim[frameIndex];
            *yOffset = penguin_transition_unhealthy_offsets[frameIndex];
            break;
        case TRANSITION_TO_HEALTHY:
            frameIndex = (FRAME_COUNT - 1) - currentFrame;
            *frameData = penguin_transition_unhealthy_anim[frameIndex];
            *yOffset = penguin_transition_unhealthy_offsets[frameIndex];
            break;
        default:
            *frameData = penguin_idle_anim[frameIndex];
            *yOffset = penguin_idle_offsets[frameIndex];
            break;
    }
}

void refreshCurrentFrame(void)
{
    if (currentAnimation == DEAD) {
        drawDeathScreen();
        return;
    }

    const uint8_t* frameData = nullptr;
    int yOffset = 0;
    getCurrentAnimationFrame(&frameData, &yOffset);
    if (frameData != nullptr) {
        drawFrameRaw(frameData, yOffset);
    }
}

void setAnimation(AnimationState newAnim)
{
    if (currentAnimation == DEAD) return;

    if (currentAnimation == newAnim && currentFrame == 0) return;

    Serial.print(F("[ANIM] Transicion: "));
    Serial.print(getAnimationName(currentAnimation));
    Serial.print(F(" -> "));
    Serial.println(getAnimationName(newAnim));

    currentAnimation = newAnim;
    currentFrame = 0;
    sleepHoldFrame = false;

    playAnimationSound(newAnim);
    refreshCurrentFrame();
}

AnimationState getCurrentBaseState(void)
{
    if (hasLightFeature() && isDark) {
        return SLEEP;
    }
    return visualUnhealthy ? IDLE_UNHEALTHY : IDLE;
}

bool tryStartHealthTransition(void)
{
    if (!isHatched || currentAnimation == DEAD || currentAnimation == POP) return false;
    if (hasLightFeature() && isDark) return false;
    if (!hasCO2Feature()) return false;

    if (co2High && !visualUnhealthy) {
        setAnimation(TRANSITION_TO_UNHEALTHY);
        return true;
    }

    if (!co2High && visualUnhealthy) {
        setAnimation(TRANSITION_TO_HEALTHY);
        return true;
    }

    return false;
}

void goToBaseState(void)
{
    if (tryStartHealthTransition()) return;
    setAnimation(getCurrentBaseState());
}

/* ========================================================================== */
/* LIFE CYCLE: DEATH SEQUENCE & CLEAN RESET                                   */
/* ========================================================================== */

/**
 * @brief Inicia la secuencia de muerte de la criatura.
 *
 * Ejecuta obligatoriamente la animación gráfica de 'POP' (15 frames de explosión)
 * en el display OLED antes de presentar la pantalla de defunción.
 *
 * @param reason Causa que motivó el fallecimiento.
 */
void triggerDeath(DeathReason reason)
{
    if (currentAnimation == POP || currentAnimation == DEAD) {
        return; // Ya está en proceso de fallecimiento
    }

    pendingDeathReason = reason;

    Serial.print(F("[DEATH] Secuencia de muerte iniciada. Causa: "));
    switch (reason) {
        case DEATH_STARVATION: Serial.println(F("INANICION (3 min sin comer)")); break;
        case DEATH_OVERFED:    Serial.println(F("SOBREALIMENTACION (POP por spam)")); break;
        case DEATH_CO2:        Serial.println(F("ASFIXIA (Exceso continuo de CO2)")); break;
        case DEATH_EXHAUSTION: Serial.println(F("AGOTAMIENTO (Falta de descanso)")); break;
        default:               Serial.println(F("DESCONOCIDA")); break;
    }

    stopSound();

    // Transición directa al estado POP para ejecutar la animación completa
    currentAnimation = POP;
    currentFrame = 0;
    sleepHoldFrame = false;

    playAnimationSound(POP);
    refreshCurrentFrame();
}

void drawDeathScreen(void)
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Encabezado centrado en la franja amarilla (128x16 px, y=0..15)
    // Texto de 17 caracteres (102 px de ancho, 8 px de alto)
    // Centrado horizontal: (128 - 102) / 2 = x 13
    // Centrado vertical:   (16 - 8) / 2 = y 4
    display.setTextSize(1);
    display.setCursor(13, 4);
    display.println(F("=== GAME OVER ==="));

    // Franja azul inferior: comienza en y=16 (hasta y=63)
    // Iniciamos en y=18 para no tocar el borde ni el gap intercolor

    // 1. Causa de la muerte (y = 18..25)
    display.setCursor(0, 18);
    display.print(F("Causa: "));
    switch (deathReason) {
        case DEATH_STARVATION: display.println(F("INANICION 3m")); break;
        case DEATH_OVERFED:    display.println(F("EXPLOTO (POP)!")); break;
        case DEATH_CO2:        display.println(F("ASFIXIA (CO2)")); break;
        case DEATH_EXHAUSTION: display.println(F("AGOTAMIENTO")); break;
        default:               display.println(F("DESCONOCIDA")); break;
    }

    // 2. Tiempo vivido (y = 29..36)
    uint32_t aliveSeconds = (deathTime >= hatchTime && hatchTime > 0) ? ((deathTime - hatchTime) / 1000UL) : 0;
    display.setCursor(0, 29);
    display.print(F("Tiempo vivo: "));
    display.print(aliveSeconds);
    display.println(F("s"));

    // 3. Caricias y Comidas (y = 40..47)
    int32_t finalScore = static_cast<int32_t>((aliveSeconds * 10UL) + (totalFeeds * 15UL) + (totalPets * 20UL));
    if (deathReason == DEATH_OVERFED || deathReason == DEATH_STARVATION) {
        finalScore = (finalScore > 50) ? (finalScore - 50) : 0;
    }
    if (finalScore < 0) finalScore = 0;

    display.setCursor(0, 40);
    display.print(F("Caricias: "));
    display.print(totalPets);
    display.print(F(" | Com: "));
    display.println(totalFeeds);

    // 4. Puntuación (y = 52..59)
    display.setCursor(0, 52);
    display.print(F("PUNTOS: "));
    display.print(finalScore);
    display.println(F(" pts"));

    display.display();
}

/**
 * @brief Restablece de forma absoluta e incondicional todos los estados del juego.
 *
 * Sin persistencia en EEPROM/Preferences/NVS: ciclo único y limpio desde cero.
 */
void resetToEggState(void)
{
    Serial.println(F("[STATE] Reset -> EGG (Todos los contadores limpios desde cero)"));

    uint32_t now = millis();

    stopSound();

    isHatched = false;
    birthTriggered = false;
    feedRequested = false;
    sleepHoldFrame = false;
    rapidFeedCount = 0;
    firstFeedPressTime = 0;
    lastTouchDetected = false;
    touchSensorInitialized = false;

    co2High = false;
    currentCO2ppm = 400;
    displaySmoothCO2 = 400;
    visualUnhealthy = false;

    pendingDeathReason = DEATH_NONE;
    deathReason = DEATH_NONE;
    deathTime = 0;
    hatchTime = 0;
    highCO2AccumulatedMs = 0;
    continuousAwakeTime = 0;
    currentHappiness = MAX_HAPPINESS;
    totalPets = 0;
    totalFeeds = 0;

    eggStartTime = now;
    lastFeedTestTime = now;
    lastCO2CheckTime = now;
    lastAwakeCheckTime = now;
    lastHappinessDecayTime = now;
    lastButtonTime = now;
    lastLightReadTime = now;
    lastTouchReadTime = now;
    lastCO2LogicTime = now;
    lastFrameTime = now;
    lastHungerSoundTime = now;
    lastSleepySoundTime = now;
    lastCoughSoundTime = now;

    lastButtonPhysicalState = HIGH;

    currentAnimation = IDLE_EGG;
    currentFrame = 0;

    playAnimationSound(IDLE_EGG);
    refreshCurrentFrame();
}

/* ========================================================================== */
/* PERIPHERAL INPUT HANDLERS                                                  */
/* ========================================================================== */

bool isTouchActive(void)
{
#if USE_TOUCH_SENSOR
    return digitalRead(PIN_TOUCH) == HIGH;
#else
    return false;
#endif
}

void showBootMessage(void)
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 18);
    display.println(F("GotchiLab_"));

    display.setTextSize(1);
    display.setCursor(35, 48);
    display.println(F("Iniciando..."));
    display.display();
}

static void registerFeedSpam(void)
{
    if (!isHatched || currentAnimation == IDLE_EGG || currentAnimation == BIRTH || 
        currentAnimation == POP || currentAnimation == DEAD) {
        rapidFeedCount = 0;
        firstFeedPressTime = 0;
        feedRequested = false;
        return;
    }

    uint32_t now = millis();

    // Período de gracia tras nacer
    if (now - hatchTime < NEWBORN_GRACE_PERIOD_MS) {
        rapidFeedCount = 0;
        firstFeedPressTime = 0;
        return;
    }

    if (firstFeedPressTime == 0 || (now - firstFeedPressTime > FEED_SPAM_WINDOW_MS)) {
        firstFeedPressTime = now;
        rapidFeedCount = 1;
        Serial.print(F("[FEED] Ventana spam: "));
        Serial.println(rapidFeedCount);
    } else {
        rapidFeedCount++;
        Serial.print(F("[FEED] Conteo spam: "));
        Serial.println(rapidFeedCount);
    }

    if (rapidFeedCount >= FEED_SPAM_TRIGGER) {
        Serial.println(F("[BUTTON] ¡¡¡ SOBREALIMENTACION DETECTADA -> POP !!!"));
        rapidFeedCount = 0;
        firstFeedPressTime = 0;
        feedRequested = false;
        triggerDeath(DEATH_OVERFED);
    }
}

void handleButton(void)
{
#if USE_BUTTON_SENSOR
    if (currentAnimation == POP || currentAnimation == DEAD) return;

    bool pressed = isButtonPressed();
    bool state = pressed ? LOW : HIGH;

    if (lastButtonPhysicalState == HIGH && state == LOW) {
        uint32_t now = millis();

        if (now - lastButtonTime > DEBOUNCE_MS) {
            lastButtonTime = now;

            if (currentAnimation == IDLE_EGG && !birthTriggered) {
                Serial.println(F("[BUTTON] Pulsador en cascaron -> ECLOSION (BIRTH)"));
                birthTriggered = true;
                setAnimation(BIRTH);
                lastButtonPhysicalState = state;
                return;
            }

            if (!isHatched ||
                currentAnimation == BIRTH ||
                currentAnimation == POP ||
                currentAnimation == DEAD ||
                isTransitionAnimation(currentAnimation)) {
                rapidFeedCount = 0;
                firstFeedPressTime = 0;
                feedRequested = false;
                lastButtonPhysicalState = state;
                return;
            }

            registerFeedSpam();

            if (currentAnimation == POP || currentAnimation == DEAD) {
                lastButtonPhysicalState = state;
                return;
            }

            if (currentAnimation == IDLE || currentAnimation == IDLE_UNHEALTHY) {
                feedRequested = true;
                lastFeedTestTime = now;
                lastHungerSoundTime = now;
                totalFeeds++;
                setAnimation(FEED);
            }
        }
    }

    lastButtonPhysicalState = state;
#endif
}

void handleLightSensor(void)
{
#if USE_LIGHT_SENSOR
    if (currentAnimation == POP || currentAnimation == DEAD) return;

    uint32_t now = millis();
    if (now - lastLightReadTime < LIGHT_READ_INTERVAL_MS) return;
    lastLightReadTime = now;

    uint16_t lightRaw = analogRead(PIN_LDR);
    float voltage = (static_cast<float>(lightRaw) * 3.3f) / 4095.0f;
    isDark = (lightRaw < LDR_DARK_THRESHOLD);

#if DEBUG_LDR_CALIBRATION
    static uint32_t lastLdrDebugTime = 0;
    if (now - lastLdrDebugTime >= LDR_DEBUG_INTERVAL_MS) {
        lastLdrDebugTime = now;
        Serial.print(F("[LDR DEBUG] ADC Raw: "));
        Serial.print(lightRaw);
        Serial.print(F("/4095 | Voltaje: "));
        Serial.print(voltage, 3);
        Serial.print(F(" V | Umbral: "));
        Serial.print(LDR_DARK_THRESHOLD);
        Serial.print(F(" | Estado: "));
        Serial.println(isDark ? F("OSCURO (DORMIR)") : F("ILUMINADO (DESPIERTO)"));
    }
#endif

    if (!isHatched) {
        lastDarkState = isDark;
        return;
    }

    if (isDark != lastDarkState) {
        Serial.print(F("[LIGHT] Cambio: "));
        Serial.println(isDark ? F("OSCURO -> Sueno") : F("LUZ -> Despertar"));

        if (isDark) {
            feedRequested = false;
            if (currentAnimation == IDLE || currentAnimation == IDLE_UNHEALTHY) {
                setAnimation(SLEEP);
            }
        } else {
            sleepHoldFrame = false;
            if (currentAnimation == SLEEP) {
                goToBaseState();
            }
        }

        lastDarkState = isDark;
    }
#endif
}

void handleTouchSensor(void)
{
#if USE_TOUCH_SENSOR
    if (currentAnimation == POP || currentAnimation == DEAD) return;

    uint32_t now = millis();
    if (now - lastTouchReadTime < 50) return;
    lastTouchReadTime = now;

    bool touchDetected = isTouchActive();

    if (touchDetected != lastTouchDetected) {
        if (!touchSensorInitialized) {
            lastTouchDetected = touchDetected;
            touchSensorInitialized = true;
            return;
        }
    }

    if (!isHatched) {
        if (currentAnimation == IDLE_EGG &&
            touchDetected &&
            !lastTouchDetected &&
            !birthTriggered) {
            Serial.println(F("[TOUCH] Caricia en cascaron -> ECLOSION (BIRTH)"));
            birthTriggered = true;
            setAnimation(BIRTH);
        }

        lastTouchDetected = touchDetected;
        return;
    }

    if (currentAnimation == POP ||
        currentAnimation == PET ||
        currentAnimation == FEED ||
        currentAnimation == SLEEP ||
        isTransitionAnimation(currentAnimation)) {
        lastTouchDetected = touchDetected;
        return;
    }

    if (hasLightFeature() && isDark) {
        lastTouchDetected = touchDetected;
        return;
    }

    if ((currentAnimation == IDLE || currentAnimation == IDLE_UNHEALTHY) &&
        touchDetected && !lastTouchDetected) {
        totalPets++;
        if (currentHappiness < MAX_HAPPINESS) currentHappiness++;
        setAnimation(PET);
    }

    lastTouchDetected = touchDetected;
#endif
}

void handleAutoHatch(void)
{
    if (!isHatched && !birthTriggered && currentAnimation == IDLE_EGG) {
        uint32_t elapsed = millis() - eggStartTime;
#if (!USE_TOUCH_SENSOR)
        if (elapsed >= AUTO_HATCH_DELAY_MS) {
            Serial.print(F("[AUTO] Eclosion automatica (Sin Touch) tras "));
            Serial.print(elapsed);
            Serial.println(F(" ms"));
            birthTriggered = true;
            setAnimation(BIRTH);
        }
#else
        // Respaldo de seguridad: si pasan 8s y no hay sensor táctil conectado, eclosiona para entrar al juego
        if (elapsed >= 8000UL) {
            Serial.print(F("[AUTO] Eclosion por tiempo de espera (8s) tras "));
            Serial.print(elapsed);
            Serial.println(F(" ms"));
            birthTriggered = true;
            setAnimation(BIRTH);
        }
#endif
    }
}

void handleCO2Sensor(void)
{
#if USE_CO2_SENSOR
    if (currentAnimation == POP || currentAnimation == DEAD) return;

    uint32_t now = millis();
    if (now - lastCO2LogicTime < CO2_READ_INTERVAL_MS) return;
    lastCO2LogicTime = now;

    if (!hasCO2Feature()) {
        co2High = false;
        currentCO2ppm = 400;
        return;
    }

    updateSensors();
    currentCO2ppm = getCO2();

    bool newCo2High = co2High;

    if (!co2High && currentCO2ppm >= CO2_HIGH_ON_PPM) {
        Serial.print(F("[CO2] ALERTA: Atmosfera contaminada: "));
        Serial.print(currentCO2ppm);
        Serial.println(F(" ppm"));
        newCo2High = true;
    } else if (co2High && currentCO2ppm <= CO2_HIGH_OFF_PPM) {
        Serial.print(F("[CO2] Atmosfera limpia restablecida: "));
        Serial.print(currentCO2ppm);
        Serial.println(F(" ppm"));
        newCo2High = false;
    }

    co2High = newCo2High;

    if (!isHatched) return;

    if ((currentAnimation == IDLE || currentAnimation == IDLE_UNHEALTHY) &&
        !(hasLightFeature() && isDark)) {
        tryStartHealthTransition();
    }
#endif
}

void handleMutePin(void)
{
#if USE_BUZZER
    bool pinState = digitalRead(PIN_MUTE);
    uint32_t now = millis();

    // Detección de flanco de bajada (puente a masa / GND)
    if (lastMutePhysicalState == HIGH && pinState == LOW) {
        if (now - lastMuteToggleTime > MUTE_DEBOUNCE_MS) {
            lastMuteToggleTime = now;
            isMuted = !isMuted;

            if (isMuted) {
                stopSound();
                Serial.println(F("[AUDIO] Silenciado (Mute ON)"));
            } else {
                Serial.println(F("[AUDIO] Sonido Habilitado (Mute OFF)"));
                ledcWriteTone(BUZZER_CHANNEL, 880);
                ledcWrite(BUZZER_CHANNEL, BUZZER_VOLUME);
                delay(40);
                ledcWriteTone(BUZZER_CHANNEL, 0);
                ledcWrite(BUZZER_CHANNEL, 0);
            }
        }
    }

    lastMutePhysicalState = pinState;
#endif
}

/* ========================================================================== */
/* VITAL SIGNS & PERIODIC CONDITION CHECKS                                    */
/* ========================================================================== */

void updateGameStats(void)
{
    if (!isHatched || currentAnimation == POP || currentAnimation == DEAD || 
        currentAnimation == BIRTH || currentAnimation == IDLE_EGG) {
        return;
    }

    uint32_t now = millis();

    // 1. Inanición
    if (hasButtonFeature()) {
        if (now - lastFeedTestTime >= STARVATION_TIME_MS) {
            triggerDeath(DEATH_STARVATION);
            return;
        }
    }

    // 2. Intoxicación por CO2 (solo si el sensor está activo y no anulado)
    if (hasCO2Feature()) {
        uint32_t dt = (lastCO2CheckTime > 0) ? (now - lastCO2CheckTime) : 0;
        lastCO2CheckTime = now;

        if (co2High) {
            highCO2AccumulatedMs += dt;
            if (highCO2AccumulatedMs >= MAX_CO2_EXPOSURE_MS) {
                triggerDeath(DEATH_CO2);
                return;
            }
        } else {
            uint32_t rec = dt * CO2_CLEAN_RECOVERY_MULTIPLIER;
            if (highCO2AccumulatedMs > rec) {
                highCO2AccumulatedMs -= rec;
            } else {
                highCO2AccumulatedMs = 0;
            }
        }
    }

    // 3. Agotamiento extremo
    if (hasLightFeature()) {
        uint32_t dt = (lastAwakeCheckTime > 0) ? (now - lastAwakeCheckTime) : 0;
        lastAwakeCheckTime = now;

        if (currentAnimation != SLEEP) {
            continuousAwakeTime += dt;
            if (continuousAwakeTime >= MAX_TIME_AWAKE_MS) {
                triggerDeath(DEATH_EXHAUSTION);
                return;
            }
        } else {
            uint32_t sleepGain = dt * SLEEP_RECOVERY_MULTIPLIER;
            if (continuousAwakeTime > sleepGain) {
                continuousAwakeTime -= sleepGain;
            } else {
                continuousAwakeTime = 0;
            }
        }
    }

    // 4. Decaimiento de Felicidad
    if (hasTouchFeature()) {
        if (now - lastHappinessDecayTime >= HAPPINESS_DECAY_MS) {
            lastHappinessDecayTime = now;
            if (currentHappiness > MIN_HAPPINESS) {
                currentHappiness--;
            }
        }
    }
}

void checkConditionSounds(void)
{
#if USE_BUZZER
    if (isMuted || soundPlaying) return;
    if (!isHatched || currentAnimation == POP || currentAnimation == DEAD || 
        currentAnimation == BIRTH || currentAnimation == IDLE_EGG) return;

    uint32_t now = millis();

    // 1. Tos de alarma si CO2 es peligroso
    if (hasCO2Feature() && co2High) {
        if (now - lastCoughSoundTime >= CO2_COUGH_INTERVAL_MS) {
            lastCoughSoundTime = now;
            playSoundSequence(SOUND_COUGH, sizeof(SOUND_COUGH) / sizeof(SOUND_COUGH[0]));
            return;
        }
    }

    if (currentAnimation == SLEEP) return;

    // 2. Rugido de hambre
    if (hasButtonFeature()) {
        if (now - lastFeedTestTime >= HUNGER_ALERT_TIME_MS) {
            if (now - lastHungerSoundTime >= HUNGER_SOUND_INTERVAL_MS) {
                lastHungerSoundTime = now;
                playSoundSequence(SOUND_HUNGER_GROWL, sizeof(SOUND_HUNGER_GROWL) / sizeof(SOUND_HUNGER_GROWL[0]));
                return;
            }
        }
    }

    // 3. Melodía somnolienta
    if (hasLightFeature() && !isDark) {
        if (continuousAwakeTime >= SLEEP_ALERT_TIME_MS) {
            if (now - lastSleepySoundTime >= SLEEP_SOUND_INTERVAL_MS) {
                lastSleepySoundTime = now;
                playSoundSequence(SOUND_SLEEPY, sizeof(SOUND_SLEEPY) / sizeof(SOUND_SLEEPY[0]));
                return;
            }
        }
    }
#endif
}

/* ========================================================================== */
/* SETUP & LOOP                                                               */
/* ========================================================================== */

void setup(void)
{
    Serial.begin(115200);
    delay(100);

    Serial.println();
    Serial.println(F("=================================================="));
    Serial.println(F("                 GOTCHILAB_ START                 "));
    Serial.println(F("=================================================="));

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

#if USE_TOUCH_SENSOR
    pinMode(PIN_TOUCH, INPUT);
#endif

#if USE_LIGHT_SENSOR
    pinMode(PIN_LDR, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_LDR, ADC_11db);
#endif

#if USE_BUZZER
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
    ledcAttachPin(PIN_BUZZER, BUZZER_CHANNEL);
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWrite(BUZZER_CHANNEL, 0);
    pinMode(PIN_MUTE, INPUT_PULLUP);
#endif

#if USE_CO2_SENSOR
    // Configurar y comprobar jumper de modo ferias / bypass de CO2
    pinMode(PIN_FAIR_MODE, INPUT_PULLUP);
    delay(10);
    bool fairModeActive = (digitalRead(PIN_FAIR_MODE) == LOW);
    if (fairModeActive) {
        Serial.println(F("[BOOT] >>> MODO FERIAS ACTIVO (PIN_FAIR_MODE a GND) <<<"));
        Serial.println(F("[BOOT] Bypass de CO2 activo: SCD30 omitido para demostraciones."));
    }
    initSensors(fairModeActive);
#else
    initSensors(true);
#endif

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("[ERROR CRITICO] Display OLED SSD1306 no encontrado en I2C!"));
        while (true) {
            delay(1000);
        }
    }

    display.clearDisplay();
    display.display();

    showBootMessage();
    delay(2000);

#if USE_LIGHT_SENSOR
    {
        uint16_t lightVal = analogRead(PIN_LDR);
        isDark = (lightVal < LDR_DARK_THRESHOLD);
        lastDarkState = isDark;
    }
#else
    isDark = false;
    lastDarkState = false;
#endif

#if USE_CO2_SENSOR
    if (hasCO2Feature()) {
        updateSensors();
        currentCO2ppm = getCO2();
    } else {
        currentCO2ppm = 400;
    }
#else
    currentCO2ppm = 400;
#endif

    resetToEggState();

    Serial.println(F("[BOOT] Sistema inicializado exitosamente."));
}

void loop(void)
{
    uint32_t now = millis();

    // Gestión de estado DEAD (Pantalla de Game Over con reinicio asistido)
    if (currentAnimation == DEAD) {
        updateSound();

        bool restartRequested = (now - deathTime >= 1500UL) && (isButtonPressed() || isTouchActive());
        if (restartRequested || (now - deathTime >= DEATH_SCREEN_DURATION_MS)) {
            resetToEggState();
        }
        return;
    }

#if USE_LIGHT_SENSOR
    handleLightSensor();
#endif

#if USE_TOUCH_SENSOR
    handleTouchSensor();
#endif
    handleAutoHatch();

#if USE_BUTTON_SENSOR
    handleButton();
#endif

#if USE_CO2_SENSOR
    handleCO2Sensor();
#endif

    handleMutePin();
    updateGameStats();
    checkConditionSounds();
    updateSound();

    // Motor de Renderizado Gráfico a 5 FPS (200 ms por cuadro)
    if (now - lastFrameTime >= FRAME_TIME_MS) {
        lastFrameTime = now;

        bool advance = true;

        if (currentAnimation == SLEEP && hasLightFeature() && isDark) {
            if (currentFrame >= SLEEP_HOLD_FRAME) {
                currentFrame = SLEEP_HOLD_FRAME;
                sleepHoldFrame = true;
                advance = false;
            }
        } else {
            sleepHoldFrame = false;
        }

        if (advance) {
            currentFrame++;

            if (currentFrame >= FRAME_COUNT) {
                if (currentAnimation == IDLE_EGG) {
                    currentFrame = 0;
                }
                else if (currentAnimation == BIRTH) {
                    isHatched = true;
                    birthTriggered = false;
                    hatchTime = now;
                    lastFeedTestTime = now;
                    lastCO2CheckTime = now;
                    lastAwakeCheckTime = now;
                    lastHappinessDecayTime = now;
                    lastHungerSoundTime = now;
                    lastSleepySoundTime = now;
                    lastCoughSoundTime = now;
                    continuousAwakeTime = 0;
                    highCO2AccumulatedMs = 0;
                    currentHappiness = MAX_HAPPINESS;
                    rapidFeedCount = 0;
                    firstFeedPressTime = 0;
                    feedRequested = false;

                    goToBaseState();
                    return;
                }
                else if (currentAnimation == FEED && feedRequested) {
                    feedRequested = false;
                    goToBaseState();
                    return;
                }
                else if (currentAnimation == PET) {
                    goToBaseState();
                    return;
                }
                else if (currentAnimation == POP) {
                    // Secuencia gráfica de explosión/desvanecimiento completada obligatoriamente:
                    // Ahora pasamos formalmente al estado DEAD y dibujamos la pantalla de Game Over
                    currentAnimation = DEAD;
                    deathReason = pendingDeathReason;
                    deathTime = now;
                    playAnimationSound(DEAD);
                    drawDeathScreen();
                    return;
                }
                else if (currentAnimation == SLEEP) {
                    if (hasLightFeature() && isDark) {
                        currentFrame = SLEEP_HOLD_FRAME;
                        sleepHoldFrame = true;
                    } else {
                        goToBaseState();
                        return;
                    }
                }
                else if (currentAnimation == TRANSITION_TO_UNHEALTHY) {
                    visualUnhealthy = true;
                    setAnimation(IDLE_UNHEALTHY);
                    return;
                }
                else if (currentAnimation == TRANSITION_TO_HEALTHY) {
                    visualUnhealthy = false;
                    setAnimation(IDLE);
                    return;
                }
                else {
                    currentFrame = 0;
                }
            }
        }

        refreshCurrentFrame();
    }
}