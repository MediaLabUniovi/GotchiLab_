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
    TRANSITION_TO_HEALTHY = 9
};

AnimationState currentAnimation = IDLE_EGG;
static uint8_t currentFrame = 0;

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

const Note* currentSound = nullptr;
uint8_t currentSoundLength = 0;
uint8_t currentSoundIndex = 0;
bool soundPlaying = false;
uint32_t soundLastChange = 0;

// =====================================================
// VARIABLES DE ESTADO
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
bool co2High = false;

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

// =====================================================
// PROTOTIPOS
// =====================================================

const char* getAnimationName(AnimationState anim);
void playAnimationSound(AnimationState anim);
bool hasLightFeature();
bool hasCO2Feature();
bool isTransitionAnimation(AnimationState anim);
bool isBusyAnimation(AnimationState anim);
void drawFrameRaw(const uint8_t* frameData, int yOffset);
void getCurrentAnimationFrame(const uint8_t** frameData, int* yOffset);
void refreshCurrentFrame();
void setAnimation(AnimationState newAnim);
AnimationState getCurrentBaseState();
bool tryStartHealthTransition();
void goToBaseState();
void resetToEggState();
bool isTouchActive();
void showBootMessage();
void registerFeedSpam();
void handleButton();
void handleLightSensor();
void handleTouchSensor();
void handleAutoHatch();
void handleCO2Sensor();
void playSoundSequence(const Note* sequence, uint8_t length);
void stopSound();
void updateSound();

// =====================================================
// SONIDO
// =====================================================

void playSoundSequence(const Note* sequence, uint8_t length)
{
#if USE_BUZZER
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
    }
}

// =====================================================
// HELPERS DE LOGICA
// =====================================================

bool hasLightFeature()
{
    return USE_LIGHT_SENSOR;
}

bool hasCO2Feature()
{
    return USE_CO2_SENSOR;
}

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
            anim == TRANSITION_TO_HEALTHY);
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
    const uint8_t* frameData;
    int yOffset;
    getCurrentAnimationFrame(&frameData, &yOffset);
    drawFrameRaw(frameData, yOffset);
}

void setAnimation(AnimationState newAnim)
{
    if (currentAnimation == newAnim && currentFrame == 0) {
        Serial.print("[ANIM] Ignorando cambio a ");
        Serial.println(getAnimationName(newAnim));
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
    if (!isHatched) return false;
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
// BOTON = COMER
// MUCHAS PULSACIONES = POP
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
        setAnimation(POP);
    }
}

void handleButton()
{
#if USE_BUTTON_SENSOR
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

            if (currentAnimation == POP) {
                lastButtonPhysicalState = state;
                return;
            }

            if (currentAnimation == IDLE || currentAnimation == IDLE_UNHEALTHY) {
                Serial.println("[BUTTON] FEED");
                feedRequested = true;
                setAnimation(FEED);
            }
        }
    }

    lastButtonPhysicalState = state;
#endif
}

// =====================================================
// LUZ = DORMIR
// Antes de nacer no afecta
// =====================================================

void handleLightSensor()
{
#if USE_LIGHT_SENSOR
    uint32_t now = millis();

    if (now - lastLightReadTime < LIGHT_READ_INTERVAL_MS) return;
    lastLightReadTime = now;

    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    isDark = lightValue < LIGHT_THRESHOLD;

    Serial.print("[LIGHT] Valor: ");
    Serial.print(lightValue);
    Serial.print(" (umbral: ");
    Serial.print(LIGHT_THRESHOLD);
    Serial.print(") -> ");
    Serial.println(isDark ? "OSCURO" : "ILUMINADO");

    if (!isHatched) {
        lastDarkState = isDark;
        return;
    }

    if (isDark != lastDarkState) {
        Serial.print("[LIGHT] Cambio de estado: ");
        Serial.println(isDark ? "OSCURO -> Iniciando sueño" : "LUZ -> Despertando");
        
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
                goToBaseState();
            }
        }

        lastDarkState = isDark;
    }
#endif
}

// =====================================================
// TOUCH =
// - Si no ha nacido: toque -> BIRTH
// - Si ya nacio: toque -> PET
// No reacciona a la lectura inicial, solo a cambios
// =====================================================

void handleTouchSensor()
{
#if USE_TOUCH_SENSOR
    uint32_t now = millis();

    if (now - lastTouchReadTime < 50) return;
    lastTouchReadTime = now;

    bool touchDetected = isTouchActive();

    if (touchDetected != lastTouchDetected) {
        Serial.print("[TOUCH] Estado: ");
        Serial.println(touchDetected ? "ACTIVO" : "INACTIVO");

    if (!touchSensorInitialized) {
        lastTouchDetected = touchDetected;
        touchSensorInitialized = true;
        Serial.print("[TOUCH] Sensor inicializado. Estado inicial: ");
        Serial.println(touchDetected ? "ACTIVO" : "INACTIVO");
        return;
    }
    } // cierre del if(touchDetected != lastTouchDetected)

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
// Lee desde sensors.cpp
// =====================================================

void handleCO2Sensor()
{
#if USE_CO2_SENSOR
    uint32_t now = millis();

    if (now - lastCO2LogicTime < CO2_READ_INTERVAL_MS) return;
    lastCO2LogicTime = now;

    updateSensors();
    currentCO2ppm = getCO2();

    Serial.print("[CO2] ");
    Serial.print(currentCO2ppm);
    Serial.println(" ppm");

    bool newCo2High = co2High;

    if (!co2High && currentCO2ppm >= CO2_HIGH_ON_PPM) {
        Serial.print("[CO2] ¡¡¡ ALERTA: CO2 ALTO !!! ");
        Serial.print(currentCO2ppm);
        Serial.print(" >= ");
        Serial.println(CO2_HIGH_ON_PPM);
        newCo2High = true;
    } else if (co2High && currentCO2ppm <= CO2_HIGH_OFF_PPM) {
        Serial.print("[CO2] CO2 normalizado: ");
        Serial.print(currentCO2ppm);
        Serial.print(" <= ");
        Serial.println(CO2_HIGH_OFF_PPM);
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
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== GOTCHILAB START ===");
    Serial.println("[SETUP] Inicializando comunicación I2C...");

    Wire.begin(OLED_SDA, OLED_SCL);
    Serial.println("[SETUP] I2C inicializado correctamente");

#if USE_TOUCH_SENSOR
    pinMode(TOUCH_PIN, INPUT);
    Serial.println("[SETUP] Touch sensor configurado");
#endif

#if USE_LIGHT_SENSOR
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    Serial.println("[SETUP] Light sensor configurado");
#endif

#if USE_BUZZER
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWrite(BUZZER_CHANNEL, 0);
    Serial.println("[SETUP] Buzzer configurado");
#endif

    Serial.println("[SETUP] Inicializando sensores...");
    initSensors();
    Serial.println("[SETUP] Sensores inicializados");

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("[ERROR] OLED no encontrada");
        while (true) {
            delay(1000);
        }
    }
    Serial.println("[SETUP] OLED inicializado correctamente");

    display.clearDisplay();
    display.display();
    Serial.println("[SETUP] Display limpio");

    showBootMessage();
    delay(2000);

#if USE_LIGHT_SENSOR
    {
        int lightValue = analogRead(LIGHT_SENSOR_PIN);
        isDark = lightValue < LIGHT_THRESHOLD;
        lastDarkState = isDark;
        Serial.print("[SETUP] Luz inicial: ");
        Serial.print(lightValue);
        Serial.println(isDark ? " (OSCURO)" : " (ILUMINADO)");
    }
#else
    isDark = false;
    lastDarkState = false;
    Serial.println("[SETUP] Light sensor deshabilitado");
#endif

#if USE_CO2_SENSOR
    updateSensors();
    currentCO2ppm = getCO2();
    Serial.print("[SETUP] CO2 inicial: ");
    Serial.print(currentCO2ppm);
    Serial.println(" ppm");
#else
    currentCO2ppm = 400;
    Serial.println("[SETUP] CO2 sensor deshabilitado");
#endif

    isHatched = false;
    birthTriggered = false;
    currentAnimation = IDLE_EGG;
    currentFrame = 0;
    lastTouchDetected = false;
    touchSensorInitialized = false;

    co2High = false;
    visualUnhealthy = false;

    eggStartTime = millis();
    Serial.println("[SETUP] Estado inicial de variables configurado");

    playAnimationSound(currentAnimation);
    refreshCurrentFrame();

    lastFrameTime = millis();
    lastLightReadTime = millis();
    lastTouchReadTime = millis();
    lastCO2LogicTime = millis();

    Serial.println("[SETUP] Timers inicializados");
    Serial.println("[SETUP] ======================================");
    Serial.println("=== READY ===");
    Serial.println("[SETUP] ======================================");
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    uint32_t now = millis();
    
    static uint32_t lastStatusPrint = 0;
    if (now - lastStatusPrint > 10000) {
        lastStatusPrint = now;
        Serial.print("[STATUS] Tiempo: ");
        Serial.print(now / 1000);
        Serial.print("s | Anim: ");
        Serial.print(getAnimationName(currentAnimation));
        Serial.print(" | Frame: ");
        Serial.print(currentFrame);
        Serial.print("/");
        Serial.print(FRAME_COUNT);
        Serial.print(" | Eclosionado: ");
        Serial.println(isHatched ? "SI" : "NO");
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
                    resetToEggState();
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

                else if (currentAnimation == IDLE_UNHEALTHY) {
                    currentFrame = 0;
                }

                else {
                    currentFrame = 0;
                }
            }
        }

        refreshCurrentFrame();
    }
}