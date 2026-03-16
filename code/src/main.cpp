#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------------------
// Config pantalla OLED
// ------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

#define OLED_SDA 22
#define OLED_SCL 21

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ------------------------
// Config botón
// ------------------------
#define BUTTON_PIN 33

// ------------------------
// Config sensor distancia HC-SR04
// ------------------------
#define TRIG_PIN 14
#define ECHO_PIN 27
#define PET_DISTANCE_CM 5.0f
#define DISTANCE_READ_INTERVAL_MS 120

// ------------------------
// Config sensor de luz
// ------------------------
#define LIGHT_SENSOR_PIN 34
#define LIGHT_THRESHOLD 1500
#define LIGHT_READ_INTERVAL_MS 500

// ------------------------
// Config buzzer
// ------------------------
#define BUZZER_PIN 26
#define BUZZER_CHANNEL 0
#define BUZZER_RESOLUTION 8
#define BUZZER_VOLUME 40

// ------------------------
// Timings y estados
// ------------------------
static const uint32_t DEBOUNCE_MS = 180;

uint32_t lastButtonTime = 0;
bool lastButtonState = HIGH;

uint32_t lastLightReadTime = 0;
bool isDark = false;
bool lastDarkState = false;

uint32_t lastDistanceReadTime = 0;
bool lastPetDetected = false;
bool distanceSensorInitialized = false;   // <- nuevo

// ------------------------
// Control especial de SLEEP
// ------------------------
bool sleepHoldFrame = false;
static const uint8_t SLEEP_HOLD_FRAME = 10;

// ------------------------
// Control especial de FEED
// ------------------------
bool feedRequested = false;

// ------------------------
// Control de nacimiento / huevo
// ------------------------
bool isHatched = false;
bool birthTriggered = false;

// ------------------------
// Control especial de POP por spam de comida
// ------------------------
static const uint8_t FEED_SPAM_TRIGGER = 5;          // nº de pulsaciones
static const uint32_t FEED_SPAM_WINDOW_MS = 2200;    // ventana de tiempo
uint8_t rapidFeedCount = 0;
uint32_t firstFeedPressTime = 0;

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

const Note* currentSound = nullptr;
uint8_t currentSoundLength = 0;
uint8_t currentSoundIndex = 0;
bool soundPlaying = false;
uint32_t soundLastChange = 0;

void playSoundSequence(const Note* sequence, uint8_t length)
{
    currentSound = sequence;
    currentSoundLength = length;
    currentSoundIndex = 0;
    soundPlaying = true;
    soundLastChange = 0;
}

void stopSound()
{
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWrite(BUZZER_CHANNEL, 0);
    soundPlaying = false;
    currentSound = nullptr;
    currentSoundLength = 0;
    currentSoundIndex = 0;
}

void updateSound()
{
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
}

// =====================================================
// Animaciones externas
// =====================================================

extern "C" {
    extern const uint8_t penguin_feed_anim[15][1024];
    extern const int penguin_feed_offsets[15];

    extern const uint8_t penguin_idle_anim[15][1024];
    extern const int penguin_idle_offsets[15];

    extern const uint8_t penguin_pet_anim[15][1024];
    extern const int penguin_pet_offsets[15];

    extern const uint8_t penguin_sleep_anim[15][1024];
    extern const int penguin_sleep_offsets[15];

    extern const uint8_t penguin_pop_anim[15][1024];
    extern const int penguin_pop_offsets[15];

    extern const uint8_t penguin_idle_egg_anim[15][1024];
    extern const int penguin_idle_egg_offsets[15];

    extern const uint8_t penguin_birth_anim[15][1024];
    extern const int penguin_birth_offsets[15];
}

enum AnimationState {
    FEED = 0,
    IDLE = 1,
    PET = 2,
    SLEEP = 3,
    POP = 4,
    IDLE_EGG = 5,
    BIRTH = 6
};

AnimationState currentAnimation = IDLE_EGG;
static uint8_t currentFrame = 0;

static const uint16_t FRAME_COUNT = 15;
static const uint16_t FRAME_SIZE = 1024;
static const uint16_t FRAME_TIME_MS = 200;

uint32_t lastFrameTime = 0;

// =====================================================

const char* getAnimationName(AnimationState anim)
{
    switch (anim) {
        case FEED:     return "FEED";
        case IDLE:     return "IDLE";
        case PET:      return "PET";
        case SLEEP:    return "SLEEP";
        case POP:      return "POP";
        case IDLE_EGG: return "IDLE_EGG";
        case BIRTH:    return "BIRTH";
        default:       return "UNKNOWN";
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
    }
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
    switch (currentAnimation) {
        case FEED:
            *frameData = penguin_feed_anim[currentFrame];
            *yOffset = penguin_feed_offsets[currentFrame];
            break;
        case IDLE:
            *frameData = penguin_idle_anim[currentFrame];
            *yOffset = penguin_idle_offsets[currentFrame];
            break;
        case PET:
            *frameData = penguin_pet_anim[currentFrame];
            *yOffset = penguin_pet_offsets[currentFrame];
            break;
        case SLEEP:
            *frameData = penguin_sleep_anim[currentFrame];
            *yOffset = penguin_sleep_offsets[currentFrame];
            break;
        case POP:
            *frameData = penguin_pop_anim[currentFrame];
            *yOffset = penguin_pop_offsets[currentFrame];
            break;
        case IDLE_EGG:
            *frameData = penguin_idle_egg_anim[currentFrame];
            *yOffset = penguin_idle_egg_offsets[currentFrame];
            break;
        case BIRTH:
            *frameData = penguin_birth_anim[currentFrame];
            *yOffset = penguin_birth_offsets[currentFrame];
            break;
        default:
            *frameData = penguin_idle_anim[currentFrame];
            *yOffset = penguin_idle_offsets[currentFrame];
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

void resetToEggState()
{
    Serial.println("[STATE] Reset -> EGG");

    isHatched = false;
    birthTriggered = false;
    feedRequested = false;
    sleepHoldFrame = false;
    rapidFeedCount = 0;
    firstFeedPressTime = 0;
    lastPetDetected = false;
    distanceSensorInitialized = false;

    currentAnimation = IDLE_EGG;
    currentFrame = 0;

    playAnimationSound(IDLE_EGG);
    refreshCurrentFrame();
}

void setAnimation(AnimationState newAnim)
{
    if (currentAnimation == newAnim && currentFrame == 0) {
        return;
    }

    currentAnimation = newAnim;
    currentFrame = 0;
    sleepHoldFrame = false;

    Serial.print("[ANIM] -> ");
    Serial.println(getAnimationName(currentAnimation));

    playAnimationSound(newAnim);
    refreshCurrentFrame();
}

// =====================================================
// UTILIDADES SENSOR DISTANCIA
// =====================================================

float readDistanceCM()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);

    if (duration == 0) {
        return -1.0f;
    }

    float distance = duration * 0.0343f / 2.0f;
    return distance;
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
    } else {
        rapidFeedCount++;
    }

    Serial.print("[FEED_COUNT] ");
    Serial.println(rapidFeedCount);

    if (rapidFeedCount >= FEED_SPAM_TRIGGER) {
        Serial.println("[BUTTON] POP por sobrealimentacion");
        rapidFeedCount = 0;
        firstFeedPressTime = 0;
        feedRequested = false;
        setAnimation(POP);
    }
}

void handleButton()
{
    bool state = digitalRead(BUTTON_PIN);

    if (lastButtonState == HIGH && state == LOW) {
        uint32_t now = millis();

        if (now - lastButtonTime > DEBOUNCE_MS) {
            lastButtonTime = now;

            // Mientras está en huevo o nacimiento, el botón no hace nada
            if (!isHatched || currentAnimation == IDLE_EGG || currentAnimation == BIRTH || currentAnimation == POP) {
                Serial.println("[BUTTON] ignorado (aun no ha nacido o esta en POP)");
                lastButtonState = state;
                return;
            }

            registerFeedSpam();

            if (currentAnimation == POP) {
                lastButtonState = state;
                return;
            }

            Serial.println("[BUTTON] FEED");
            feedRequested = true;
            setAnimation(FEED);
        }
    }

    lastButtonState = state;
}

// =====================================================
// LUZ = DORMIR
// Antes de nacer no afecta
// =====================================================

void handleLightSensor()
{
    uint32_t now = millis();

    if (now - lastLightReadTime < LIGHT_READ_INTERVAL_MS) return;

    lastLightReadTime = now;

    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    isDark = lightValue < LIGHT_THRESHOLD;

    Serial.print("[LIGHT] ");
    Serial.print(lightValue);
    Serial.print(" -> ");
    Serial.println(isDark ? "OSCURO" : "LUZ");

    if (!isHatched) {
        lastDarkState = isDark;
        return;
    }

    if (isDark != lastDarkState) {
        if (isDark) {
            feedRequested = false;
            setAnimation(SLEEP);
        } else {
            sleepHoldFrame = false;
        }

        lastDarkState = isDark;
    }
}

// =====================================================
// DISTANCIA =
// - Si no ha nacido: activity -> BIRTH
// - Si ya nacio: a < 5 cm -> PET
// Pero NO reacciona a la lectura inicial, solo a cambios
// =====================================================

void handleDistanceSensor()
{
    uint32_t now = millis();

    if (now - lastDistanceReadTime < DISTANCE_READ_INTERVAL_MS) return;

    lastDistanceReadTime = now;

    float distance = readDistanceCM();
    bool validDistance = (distance > 0.0f);
    bool petDetected = (distance > 0.0f && distance <= PET_DISTANCE_CM);

    if (validDistance) {
        Serial.print("[DIST] ");
        Serial.print(distance);
        Serial.println(" cm");
    } else {
        Serial.println("[DIST] sin lectura");
    }

    // Primera lectura válida: solo memoriza estado, no dispara animación
    if (!distanceSensorInitialized) {
        if (validDistance) {
            lastPetDetected = petDetected;
            distanceSensorInitialized = true;
            Serial.println("[DIST] Sensor inicializado, esperando cambio...");
        }
        return;
    }

    // -----------------------------
    // Fase huevo -> nacimiento
    // Solo si hay cambio real: antes no detectaba, ahora sí
    // -----------------------------
    if (!isHatched) {
        if (currentAnimation == IDLE_EGG && petDetected && !lastPetDetected && !birthTriggered) {
            Serial.println("[DIST] Cambio detectado -> BIRTH");
            birthTriggered = true;
            setAnimation(BIRTH);
        }

        lastPetDetected = petDetected;
        return;
    }

    // Durante POP no hacemos nada
    if (currentAnimation == POP) {
        lastPetDetected = petDetected;
        return;
    }

    if (isDark) {
        lastPetDetected = petDetected;
        return;
    }

    if (currentAnimation == FEED && feedRequested) {
        lastPetDetected = petDetected;
        return;
    }

    if (currentAnimation == SLEEP) {
        lastPetDetected = petDetected;
        return;
    }

    // Si ya está acariciando, no interrumpir PET hasta que termine
    if (currentAnimation == PET) {
        lastPetDetected = petDetected;
        return;
    }

    // Solo lanzar PET cuando aparece una detección nueva
    if (petDetected && !lastPetDetected) {
        Serial.println("[DIST] Cambio cerca -> PET");
        setAnimation(PET);
    }

    lastPetDetected = petDetected;
}

// =====================================================

void setup()
{
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== GOTCHILAB START ===");

    Wire.begin(OLED_SDA, OLED_SCL);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LIGHT_SENSOR_PIN, INPUT);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);

    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWrite(BUZZER_CHANNEL, 0);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("[ERROR] OLED no encontrada");
        while (true) {
            delay(1000);
        }
    }

    display.clearDisplay();
    display.display();

    showBootMessage();
    delay(2000);

    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    isDark = lightValue < LIGHT_THRESHOLD;
    lastDarkState = isDark;

    // Empieza SIEMPRE en huevo
    isHatched = false;
    birthTriggered = false;
    currentAnimation = IDLE_EGG;
    currentFrame = 0;
    lastPetDetected = false;
    distanceSensorInitialized = false;

    playAnimationSound(currentAnimation);
    refreshCurrentFrame();

    lastFrameTime = millis();
    lastLightReadTime = millis();
    lastDistanceReadTime = millis();

    Serial.println("=== READY ===");
}

// =====================================================

void loop()
{
    uint32_t now = millis();

    handleLightSensor();
    handleDistanceSensor();
    handleButton();
    updateSound();

    if (now - lastFrameTime >= FRAME_TIME_MS) {
        lastFrameTime = now;

        bool advance = true;

        if (currentAnimation == SLEEP && isDark) {
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

                // ---------------------------------
                // HUEVO: se queda en loop
                // ---------------------------------
                if (currentAnimation == IDLE_EGG) {
                    currentFrame = 0;
                }

                // ---------------------------------
                // NACIMIENTO -> IDLE o SLEEP
                // ---------------------------------
                else if (currentAnimation == BIRTH) {
                    isHatched = true;
                    birthTriggered = false;
                    rapidFeedCount = 0;
                    firstFeedPressTime = 0;

                    if (isDark) {
                        setAnimation(SLEEP);
                    } else {
                        setAnimation(IDLE);
                    }
                    return;
                }

                // ---------------------------------
                // FEED -> IDLE
                // ---------------------------------
                else if (currentAnimation == FEED && feedRequested) {
                    feedRequested = false;
                    setAnimation(IDLE);
                    return;
                }

                // ---------------------------------
                // PET -> IDLE
                // ---------------------------------
                else if (currentAnimation == PET) {
                    setAnimation(IDLE);
                    return;
                }

                // ---------------------------------
                // POP -> volver al inicio
                // ---------------------------------
                else if (currentAnimation == POP) {
                    resetToEggState();
                    return;
                }

                // ---------------------------------
                // SLEEP
                // ---------------------------------
                else if (currentAnimation == SLEEP) {
                    if (isDark) {
                        currentFrame = SLEEP_HOLD_FRAME;
                        sleepHoldFrame = true;
                    } else {
                        setAnimation(IDLE);
                        return;
                    }
                }

                else {
                    currentFrame = 0;
                }
            }
        }

        refreshCurrentFrame();
    }
}