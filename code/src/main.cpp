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

// =====================================================
// OLED
// =====================================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Icono Bitmap 1-bit de muslito de pollo (5x5 px)
static const unsigned char PROGMEM chicken_leg_icon[] = {
    0b01110000,
    0b11111000,
    0b11110000,
    0b01100000,
    0b00110000
};

// =====================================================
// ESTADOS DE ANIMACION
// =====================================================

enum AnimationState {
    FEED = 0,
    IDLE = 1,
    PET = 2,
    SLEEP = 3,
    POP = 4,
    IDLE_EGG = 5,
    BIRTH = 6,
    IDLE_UNHEALTHY = 7,
    TRANSITION_TO_UNHEALTHY = 8,
    TRANSITION_TO_HEALTHY = 9,
    DEAD = 10
};

// Causas de muerte
enum DeathReason {
    DEATH_NONE = 0,
    DEATH_STARVATION,    // Inanición (> 3 min sin comer)
    DEATH_OVERFED,       // Sobrealimentación (Spam de comida / POP)
    DEATH_CO2,           // Intoxicación / asfixia por CO2 prolongado
    DEATH_EXHAUSTION     // Agotamiento extremo (sin dormir en oscuridad)
};

AnimationState currentAnimation = IDLE_EGG;
static uint8_t currentFrame = 0;
DeathReason deathReason = DEATH_NONE;
uint32_t deathTime = 0;

// =====================================================
// SONIDOS
// =====================================================

struct Note {
    uint16_t freq;
    uint16_t duration;
};

const Note SOUND_FEED[] = {
    {523, 70}, {659, 70}, {784, 90}, {1047, 120}, {0, 30}
};

const Note SOUND_PET[] = {
    {659, 60}, {784, 60}, {659, 80}, {0, 25}
};

const Note SOUND_IDLE[] = {
    {523, 40}, {659, 50}, {0, 20}
};

const Note SOUND_SLEEP[] = {
    {698, 70}, {587, 80}, {494, 110}, {0, 40}
};

const Note SOUND_EGG[] = {
    {392, 70}, {440, 70}, {392, 80}, {0, 40}
};

const Note SOUND_BIRTH[] = {
    {523, 60}, {659, 60}, {784, 60}, {988, 80}, {1175, 110}, {0, 40}
};

const Note SOUND_POP[] = {
    {220, 40}, {180, 35}, {140, 35}, {90, 70}, {0, 30}
};

const Note SOUND_UNHEALTHY[] = {
    {392, 80}, {330, 90}, {0, 40}
};

const Note SOUND_TRANSITION_UNHEALTHY[] = {
    {587, 60}, {523, 70}, {440, 90}, {0, 30}
};

const Note SOUND_TRANSITION_RECOVER[] = {
    {440, 60}, {523, 70}, {659, 90}, {0, 30}
};

// Sonido trágico de derrota / muerte (Marcha fúnebre estilizada)
const Note SOUND_DEATH[] = {
    {440, 300}, {440, 300}, {440, 300}, {349, 250}, {523, 100},
    {440, 300}, {349, 250}, {523, 100}, {440, 600}, {0, 100}
};

// Sonido de alerta / advertencia crítica (bip-bip de peligro)
const Note SOUND_WARNING[] = {
    {880, 80}, {0, 40}, {880, 80}, {0, 40}
};

const Note* currentSound = nullptr;
uint8_t currentSoundLength = 0;
uint8_t currentSoundIndex = 0;
bool soundPlaying = false;
uint32_t soundLastChange = 0;

// =====================================================
// VARIABLES DE ESTADO Y ESTADÍSTICAS
// =====================================================

uint32_t lastButtonTime = 0;
bool lastButtonPhysicalState = HIGH;

uint32_t lastLightReadTime = 0;
bool isDark = false;
bool lastDarkState = false;

uint32_t lastTouchReadTime = 0;
bool lastTouchDetected = false;
bool touchSensorInitialized = false;

uint32_t lastCO2LogicTime = 0;
uint16_t currentCO2ppm = 400;
uint16_t displaySmoothCO2 = 400;
bool co2High = false;

// Tiempos para cálculo de condiciones de muerte y puntuación
uint32_t hatchTime = 0;
uint32_t lastFeedTestTime = 0;
uint32_t highCO2AccumulatedMs = 0;
uint32_t lastCO2CheckTime = 0;
uint32_t continuousAwakeTime = 0;
uint32_t lastAwakeCheckTime = 0;
uint32_t lastHappinessDecayTime = 0;

// Estadísticas de juego
int8_t currentHappiness = MAX_HAPPINESS; // 0 a 5
uint32_t totalPets = 0;
uint32_t totalFeeds = 0;

// false -> idle normal
// true  -> idle unhealthy
bool visualUnhealthy = false;

bool sleepHoldFrame = false;
bool feedRequested = false;

bool isHatched = false;
bool birthTriggered = false;

uint8_t rapidFeedCount = 0;
uint32_t firstFeedPressTime = 0;

uint32_t eggStartTime = 0;
uint32_t lastFrameTime = 0;

bool isMuted = false;
uint32_t lastMuteToggleTime = 0;
bool lastMutePhysicalState = HIGH;

// =====================================================
// PROTOTIPOS
// =====================================================

const char* getAnimationName(AnimationState anim);
void playAnimationSound(AnimationState anim);
bool hasLightFeature();
bool hasCO2Feature();
bool hasButtonFeature();
bool hasTouchFeature();
bool isTransitionAnimation(AnimationState anim);
bool isBusyAnimation(AnimationState anim);
void drawStatsOverlay();
void drawFrameRaw(const uint8_t* frameData, int yOffset);
void getCurrentAnimationFrame(const uint8_t** frameData, int* yOffset);
void refreshCurrentFrame();
void setAnimation(AnimationState newAnim);
AnimationState getCurrentBaseState();
bool tryStartHealthTransition();
void goToBaseState();
void triggerDeath(DeathReason reason);
void drawDeathScreen();
void resetToEggState();
bool isTouchActive();
void showBootMessage();
void registerFeedSpam();
void handleButton();
void handleLightSensor();
void handleTouchSensor();
void handleAutoHatch();
void handleCO2Sensor();
void handleMutePin();
void updateGameStats();
void playSoundSequence(const Note* sequence, uint8_t length);
void stopSound();
void updateSound();

// =====================================================
// SONIDO
// =====================================================

void playSoundSequence(const Note* sequence, uint8_t length)
{
#if USE_BUZZER
    if (isMuted) return; // Pingüino silenciado

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

void stopSound()
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

void updateSound()
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

// =====================================================

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

// =====================================================
// HELPERS DE LOGICA Y FEATURES
// =====================================================

bool hasLightFeature()  { return USE_LIGHT_SENSOR; }
bool hasCO2Feature()    { return USE_CO2_SENSOR; }
bool hasButtonFeature() { return USE_BUTTON_SENSOR; }
bool hasTouchFeature()  { return USE_TOUCH_SENSOR; }

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

// =====================================================
// RENDERIZADO DEL OVERLAY DE ESTADÍSTICAS (HUD)
// =====================================================

void drawStatsOverlay()
{
#if !SHOW_STATS_OVERLAY
    return; // Barra de estado oculta para modo inmersivo a ciegas: las animaciones ocupan toda la pantalla
#endif

    if (!isHatched || currentAnimation == IDLE_EGG || currentAnimation == BIRTH || currentAnimation == DEAD) {
        return;
    }

    // 1. HAMBRE (Muslitos de pollo) - Si el botón está activado
    if (hasButtonFeature()) {
        uint32_t elapsed = millis() - lastFeedTestTime;
        int hungerLevel = 3;
        if (elapsed > (STARVATION_TIME_MS * 2 / 3)) {
            hungerLevel = 1; // Crítico
        } else if (elapsed > (STARVATION_TIME_MS / 3)) {
            hungerLevel = 2; // Hambriento
        }

        for (int i = 0; i < 3; i++) {
            int x = 2 + i * 7;
            int y = 2;
            if (i < hungerLevel) {
                display.drawBitmap(x, y, chicken_leg_icon, 5, 5, SSD1306_WHITE);
            } else {
                // Dibujar contorno vacío
                display.drawRect(x, y, 5, 5, SSD1306_WHITE);
            }
        }
    }

    // 2. FELICIDAD (Corazones / Caricias) - Si el touch está activado
    if (hasTouchFeature()) {
        int xStart = 28;
        int y = 2;
        int filled = (currentHappiness > 0) ? (currentHappiness > 2 ? (currentHappiness >= 4 ? 3 : 2) : 1) : 0;
        for (int i = 0; i < 3; i++) {
            int x = xStart + i * 7;
            if (i < filled) {
                // Mini corazón lleno (5x5)
                display.drawPixel(x+1, y, SSD1306_WHITE);
                display.drawPixel(x+3, y, SSD1306_WHITE);
                display.drawFastHLine(x, y+1, 5, SSD1306_WHITE);
                display.drawFastHLine(x+1, y+2, 3, SSD1306_WHITE);
                display.drawPixel(x+2, y+3, SSD1306_WHITE);
            } else {
                // Mini corazón vacío / contorno
                display.drawPixel(x+1, y, SSD1306_WHITE);
                display.drawPixel(x+3, y, SSD1306_WHITE);
                display.drawPixel(x, y+1, SSD1306_WHITE);
                display.drawPixel(x+4, y+1, SSD1306_WHITE);
                display.drawPixel(x+2, y+3, SSD1306_WHITE);
            }
        }
    }

    // 3. BARRA DE SALUD RESPIRATORIA / AIRE LIMPIO (CO2) - Si el sensor está activado
    // Empieza a tope (100% llena) con aire limpio (400 ppm) y desciende si sube el CO2.
    // Al volver a la normalidad, sube rápidamente recuperando la salud.
    if (hasCO2Feature()) {
        int barX = 54;
        int barY = 2;
        int barW = 34;
        int barH = 5;

        display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);

        // Suavizado dinámico: cuando el CO2 baja (aire se limpia), la salud sube más rápido
        if (displaySmoothCO2 < currentCO2ppm) {
            displaySmoothCO2 += (currentCO2ppm - displaySmoothCO2 + 3) / 4;
        } else if (displaySmoothCO2 > currentCO2ppm) {
            // Recuperación rápida de salud al normalizarse el aire
            uint16_t diff = displaySmoothCO2 - currentCO2ppm;
            uint16_t step = (diff / 2) + 20;
            if (displaySmoothCO2 > currentCO2ppm + step) {
                displaySmoothCO2 -= step;
            } else {
                displaySmoothCO2 = currentCO2ppm;
            }
        }

        // Mapear salud respiratoria:
        // 400 ppm (o menos) -> Salud máxima (barra completamente llena)
        // 2000 ppm (o más) -> Salud crítica (barra vacía)
        int ppm = displaySmoothCO2;
        if (ppm < 400) ppm = 400;
        if (ppm > 2000) ppm = 2000;
        int fillW = map(ppm, 400, 2000, barW - 2, 0);
        if (fillW > 0) {
            display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);
        }
    }

    // 4. SUEÑO / ENERGÍA - Si el sensor de luz está activado
    if (hasLightFeature()) {
        int zX = 94;
        int zY = 2;
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        // Barra de energía (progreso de descanso): sube progresivamente al dormir
        uint32_t awakeTime = continuousAwakeTime;
        if (awakeTime > MAX_TIME_AWAKE_MS) awakeTime = MAX_TIME_AWAKE_MS;
        int energyRemaining = MAX_TIME_AWAKE_MS - awakeTime;
        int barW = 28;

        display.drawRect(zX, zY, barW, 5, SSD1306_WHITE);
        int fill = map(energyRemaining, 0, MAX_TIME_AWAKE_MS, 0, barW - 2);
        if (fill > 0) {
            display.fillRect(zX + 1, zY + 1, fill, 3, SSD1306_WHITE);
        }

        // Si está durmiendo, mostrar "Z" flotante o indicador
        if (currentAnimation == SLEEP) {
            display.setCursor(zX + 10, zY - 1);
            // Parpadeo sutil de Zzz
            if ((millis() / 500) % 2 == 0) {
                display.print("z");
            }
        }
    }

    // Línea divisoria discreta superior
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

    // Dibujar estadísticas HUD encima de los frames
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

void refreshCurrentFrame()
{
    if (currentAnimation == DEAD) {
        drawDeathScreen();
        return;
    }

    const uint8_t* frameData;
    int yOffset;
    getCurrentAnimationFrame(&frameData, &yOffset);
    drawFrameRaw(frameData, yOffset);
}

void setAnimation(AnimationState newAnim)
{
    if (currentAnimation == DEAD) {
        return; // No se cambia de animación si está muerto
    }

    if (currentAnimation == newAnim && currentFrame == 0) {
        return;
    }

    Serial.print("[ANIM] Transición: ");
    Serial.print(getAnimationName(currentAnimation));
    Serial.print(" -> ");
    Serial.println(getAnimationName(newAnim));

    currentAnimation = newAnim;
    currentFrame = 0;
    sleepHoldFrame = false;

    playAnimationSound(newAnim);
    refreshCurrentFrame();
}

AnimationState getCurrentBaseState()
{
    if (hasLightFeature() && isDark) {
        return SLEEP;
    }

    return visualUnhealthy ? IDLE_UNHEALTHY : IDLE;
}

bool tryStartHealthTransition()
{
    if (!isHatched || currentAnimation == DEAD) return false;
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

void goToBaseState()
{
    if (tryStartHealthTransition()) {
        return;
    }

    setAnimation(getCurrentBaseState());
}

// =====================================================
// PANTALLA DE MUERTE Y GAME OVER CON PUNTUACIÓN
// =====================================================

void triggerDeath(DeathReason reason)
{
    if (currentAnimation == DEAD) return;

    deathReason = reason;
    currentAnimation = DEAD;
    deathTime = millis();

    Serial.print("[GAME OVER] La mascota ha muerto por: ");
    switch (reason) {
        case DEATH_STARVATION: Serial.println("INANICIÓN (Sin comer 3m)"); break;
        case DEATH_OVERFED:    Serial.println("SOBREALIMENTACIÓN (POP)"); break;
        case DEATH_CO2:        Serial.println("ASFIXIA POR EXCESO DE CO2"); break;
        case DEATH_EXHAUSTION: Serial.println("AGOTAMIENTO (Sin dormir)"); break;
        default:               Serial.println("DESCONOCIDO"); break;
    }

    playAnimationSound(DEAD);
    drawDeathScreen();
}

void drawDeathScreen()
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Encabezado
    display.setTextSize(1);
    display.setCursor(22, 2);
    display.println("=== GAME OVER ===");

    // Causa de la muerte
    display.setCursor(0, 14);
    display.print("Causa: ");
    switch (deathReason) {
        case DEATH_STARVATION:
            display.println("INANICION 3m");
            break;
        case DEATH_OVERFED:
            display.println("EXPLOTO (POP)!");
            break;
        case DEATH_CO2:
            display.println("ASFIXIA (CO2)");
            break;
        case DEATH_EXHAUSTION:
            display.println("AGOTAMIENTO");
            break;
        default:
            display.println("Desconocida");
            break;
    }

    // Tiempo vivido
    uint32_t aliveSeconds = (deathTime >= hatchTime && hatchTime > 0) ? ((deathTime - hatchTime) / 1000) : 0;
    display.setCursor(0, 26);
    display.print("Tiempo vivo: ");
    display.print(aliveSeconds);
    display.println("s");

    // Cálculo de Score Final Profesional
    // Puntos por segundo vivo + bonus por cuidados (feeds y caricias) + penalización por sufrimiento
    int32_t finalScore = (aliveSeconds * 10) + (totalFeeds * 15) + (totalPets * 20);
    if (deathReason == DEATH_OVERFED || deathReason == DEATH_STARVATION) {
        finalScore = (finalScore > 50) ? (finalScore - 50) : 0;
    }
    if (finalScore < 0) finalScore = 0;

    display.setCursor(0, 38);
    display.print("Caricias: ");
    display.print(totalPets);
    display.print(" | Com: ");
    display.println(totalFeeds);

    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("PUNTUACION: ");
    display.print(finalScore);
    display.println(" pts");

    display.display();
}

void resetToEggState()
{
    Serial.println("[STATE] Reset -> EGG");

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
    visualUnhealthy = false;

    // Reset estadísticas
    deathReason = DEATH_NONE;
    deathTime = 0;
    hatchTime = 0;
    highCO2AccumulatedMs = 0;
    continuousAwakeTime = 0;
    currentHappiness = MAX_HAPPINESS;
    totalPets = 0;
    totalFeeds = 0;

    eggStartTime = millis();

    currentAnimation = IDLE_EGG;
    currentFrame = 0;

    playAnimationSound(IDLE_EGG);
    refreshCurrentFrame();
}

// =====================================================
// TOUCH
// =====================================================

bool isTouchActive()
{
#if USE_TOUCH_SENSOR
    return digitalRead(TOUCH_PIN) == HIGH;
#else
    return false;
#endif
}

// =====================================================

void showBootMessage()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 18);
    display.println("GotchiLab_");
    display.setTextSize(1);
    display.setCursor(35, 48);
    display.println("Iniciando...");
    display.display();
}

// =====================================================
// BOTON = COMER / POP
// =====================================================

void registerFeedSpam()
{
    uint32_t now = millis();

    if (firstFeedPressTime == 0 || (now - firstFeedPressTime > FEED_SPAM_WINDOW_MS)) {
        firstFeedPressTime = now;
        rapidFeedCount = 1;
        Serial.print("[FEED] Nueva ventana de conteo: ");
        Serial.println(rapidFeedCount);
    } else {
        rapidFeedCount++;
        Serial.print("[FEED] Conteo actual: ");
        Serial.println(rapidFeedCount);
    }

    if (rapidFeedCount >= FEED_SPAM_TRIGGER) {
        Serial.println("[BUTTON] ¡¡¡ POP POR SOBREALIMENTACION !!!");
        rapidFeedCount = 0;
        firstFeedPressTime = 0;
        feedRequested = false;
        triggerDeath(DEATH_OVERFED);
    }
}

void handleButton()
{
#if USE_BUTTON_SENSOR
    if (currentAnimation == DEAD) return;

    bool pressed = isButtonPressed();
    bool state = pressed ? LOW : HIGH;

    if (lastButtonPhysicalState == HIGH && state == LOW) {
        uint32_t now = millis();

        if (now - lastButtonTime > DEBOUNCE_MS) {
            lastButtonTime = now;

            if (!isHatched ||
                currentAnimation == IDLE_EGG ||
                currentAnimation == BIRTH ||
                currentAnimation == POP ||
                isTransitionAnimation(currentAnimation)) {
                Serial.println("[BUTTON] ignorado");
                lastButtonPhysicalState = state;
                return;
            }

            registerFeedSpam();

            if (currentAnimation == DEAD) {
                lastButtonPhysicalState = state;
                return;
            }

            if (currentAnimation == IDLE || currentAnimation == IDLE_UNHEALTHY) {
                Serial.println("[BUTTON] FEED");
                feedRequested = true;
                lastFeedTestTime = now;
                totalFeeds++;
                setAnimation(FEED);
            }
        }
    }

    lastButtonPhysicalState = state;
#endif
}

// =====================================================
// LUZ = DORMIR / AGOTAMIENTO
// =====================================================

void handleLightSensor()
{
#if USE_LIGHT_SENSOR
    if (currentAnimation == DEAD) return;

    uint32_t now = millis();

    if (now - lastLightReadTime < LIGHT_READ_INTERVAL_MS) return;
    lastLightReadTime = now;

    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    isDark = lightValue < LIGHT_THRESHOLD;

    if (!isHatched) {
        lastDarkState = isDark;
        return;
    }

    if (isDark != lastDarkState) {
        Serial.print("[LIGHT] Cambio: ");
        Serial.println(isDark ? "OSCURO -> Sueño" : "LUZ -> Despertar");
        
        if (isDark) {
            feedRequested = false;

            if (currentAnimation == IDLE || currentAnimation == IDLE_UNHEALTHY) {
                Serial.println("[LIGHT] Transición a SLEEP");
                setAnimation(SLEEP);
            }
        } else {
            sleepHoldFrame = false;

            if (currentAnimation == SLEEP) {
                Serial.println("[LIGHT] Saliendo de SLEEP");
                // La energía restante se conserva exactamente según lo que descansó
                goToBaseState();
            }
        }

        lastDarkState = isDark;
    }
#endif
}

// =====================================================
// TOUCH = NACER / CARICIA
// =====================================================

void handleTouchSensor()
{
#if USE_TOUCH_SENSOR
    if (currentAnimation == DEAD) return;

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
            Serial.println("[TOUCH] Toque detectado -> BIRTH");
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
        Serial.println("[TOUCH] Toque -> PET");
        totalPets++;
        if (currentHappiness < MAX_HAPPINESS) currentHappiness++;
        setAnimation(PET);
    }

    lastTouchDetected = touchDetected;
#endif
}

void handleAutoHatch()
{
#if (!USE_TOUCH_SENSOR) && AUTO_HATCH_IF_NO_TOUCH
    if (!isHatched && !birthTriggered && currentAnimation == IDLE_EGG) {
        uint32_t elapsed = millis() - eggStartTime;
        if (elapsed >= AUTO_HATCH_DELAY_MS) {
            Serial.print("[AUTO] Eclosión automática después de ");
            Serial.print(elapsed);
            Serial.println(" ms");
            birthTriggered = true;
            setAnimation(BIRTH);
        }
    }
#endif
}

// =====================================================
// CO2
// =====================================================

void handleCO2Sensor()
{
#if USE_CO2_SENSOR
    if (currentAnimation == DEAD) return;

    uint32_t now = millis();

    if (now - lastCO2LogicTime < CO2_READ_INTERVAL_MS) return;
    lastCO2LogicTime = now;

    if (!isCO2Connected()) {
        co2High = false;
        currentCO2ppm = 400;
        return;
    }

    updateSensors();
    currentCO2ppm = getCO2();

    bool newCo2High = co2High;

    if (!co2High && currentCO2ppm >= CO2_HIGH_ON_PPM) {
        Serial.print("[CO2] ¡¡¡ ALERTA: CO2 ALTO !!! ");
        Serial.println(currentCO2ppm);
        newCo2High = true;
    } else if (co2High && currentCO2ppm <= CO2_HIGH_OFF_PPM) {
        Serial.print("[CO2] CO2 normalizado: ");
        Serial.println(currentCO2ppm);
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

// =====================================================
// MUTE / SILENCIO DE BUZZER
// Puentear MUTE_PIN a GND alterna entre MUTE y SONIDO
// =====================================================

void handleMutePin()
{
#if USE_BUZZER
    bool pinState = digitalRead(MUTE_PIN);
    uint32_t now = millis();

    // Detección de flanco de bajada (conexión a tierra GND)
    if (lastMutePhysicalState == HIGH && pinState == LOW) {
        if (now - lastMuteToggleTime > MUTE_DEBOUNCE_MS) {
            lastMuteToggleTime = now;
            isMuted = !isMuted;

            if (isMuted) {
                stopSound();
                Serial.println("[AUDIO] ¡Pingüino SILENCIADO! (Mute ON)");
            } else {
                Serial.println("[AUDIO] Sonido ACTIVADO (Mute OFF)");
                // Breve pitido confirmatorio al desmutear
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

// =====================================================
// GESTIÓN DE CONDICIONES CRÍTICAS Y VIGILANCIA DE ESTADO
// =====================================================

void updateGameStats()
{
    if (!isHatched || currentAnimation == DEAD || currentAnimation == BIRTH || currentAnimation == IDLE_EGG) {
        return;
    }

    uint32_t now = millis();

    // 1. INANICIÓN (Hambre > 3 minutos) - Solo si el botón está activado
    if (hasButtonFeature()) {
        if (now - lastFeedTestTime >= STARVATION_TIME_MS) {
            triggerDeath(DEATH_STARVATION);
            return;
        }
    }

    // 2. INTOXICACIÓN POR CO2 ACUMULADO - Solo si el sensor de CO2 está activado
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
            // Se recupera más rápido al volver el aire limpio (multiplicador configurable)
            uint32_t rec = dt * CO2_CLEAN_RECOVERY_MULTIPLIER;
            if (highCO2AccumulatedMs > rec) {
                highCO2AccumulatedMs -= rec;
            } else {
                highCO2AccumulatedMs = 0;
            }
        }
    }

    // 3. AGOTAMIENTO EXTREMO & SUEÑO PROGRESIVO - Solo si el sensor de luz está activo
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
            // Dormir recupera el cansancio PROGRESIVAMENTE (llenando la barra poco a poco)
            uint32_t sleepGain = dt * SLEEP_RECOVERY_MULTIPLIER;
            if (continuousAwakeTime > sleepGain) {
                continuousAwakeTime -= sleepGain;
            } else {
                continuousAwakeTime = 0;
            }
        }
    }

    // 4. DECAIMIENTO DE FELICIDAD POR FALTA DE CARICIAS - Solo si el touch está activo
    if (hasTouchFeature()) {
        if (now - lastHappinessDecayTime >= HAPPINESS_DECAY_MS) {
            lastHappinessDecayTime = now;
            if (currentHappiness > MIN_HAPPINESS) {
                currentHappiness--;
                Serial.print("[STATS] Felicidad disminuida a: ");
                Serial.println(currentHappiness);
            }
        }
    }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== GOTCHILAB SENIOR EDITION START ===");
    Wire.begin(OLED_SDA, OLED_SCL);

#if USE_TOUCH_SENSOR
    pinMode(TOUCH_PIN, INPUT);
#endif

#if USE_LIGHT_SENSOR
    pinMode(LIGHT_SENSOR_PIN, INPUT);
#endif

#if USE_BUZZER
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWrite(BUZZER_CHANNEL, 0);
    pinMode(MUTE_PIN, INPUT_PULLUP);
#endif

    initSensors();

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("[ERROR] OLED no encontrada");
        while (true) delay(1000);
    }

    display.clearDisplay();
    display.display();

    showBootMessage();
    delay(2000);

#if USE_LIGHT_SENSOR
    {
        int lightValue = analogRead(LIGHT_SENSOR_PIN);
        isDark = lightValue < LIGHT_THRESHOLD;
        lastDarkState = isDark;
    }
#else
    isDark = false;
    lastDarkState = false;
#endif

#if USE_CO2_SENSOR
    updateSensors();
    currentCO2ppm = getCO2();
#else
    currentCO2ppm = 400;
#endif

    resetToEggState();

    lastFrameTime = millis();
    lastLightReadTime = millis();
    lastTouchReadTime = millis();
    lastCO2LogicTime = millis();
    lastCO2CheckTime = millis();
    lastAwakeCheckTime = millis();
    lastHappinessDecayTime = millis();

    Serial.println("=== GOTCHILAB LISTO ===");
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    uint32_t now = millis();

    // Pantalla de muerte / Game Over activa
    if (currentAnimation == DEAD) {
        updateSound();
        if (now - deathTime >= DEATH_SCREEN_DURATION_MS) {
            resetToEggState();
        }
        return;
    }

#if USE_LIGHT_SENSOR
    handleLightSensor();
#endif

#if USE_TOUCH_SENSOR
    handleTouchSensor();
#else
    handleAutoHatch();
#endif

#if USE_BUTTON_SENSOR
    handleButton();
#endif

#if USE_CO2_SENSOR
    handleCO2Sensor();
#endif

    // Chequear pin de silencio (mute toggle por puente a GND)
    handleMutePin();

    // Actualizar lógica de estado, inanición, felicidad y cansancio
    updateGameStats();

    updateSound();

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
                    continuousAwakeTime = 0;
                    highCO2AccumulatedMs = 0;
                    currentHappiness = MAX_HAPPINESS;
                    rapidFeedCount = 0;
                    firstFeedPressTime = 0;

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
                    triggerDeath(DEATH_OVERFED);
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