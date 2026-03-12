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
#define BUZZER_VOLUME 16

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

// ------------------------
// Control especial de SLEEP
// ------------------------
bool sleepHoldFrame = false;
static const uint8_t SLEEP_HOLD_FRAME = 10;

// ------------------------
// Control especial de FEED
// ------------------------
bool feedRequested = false;

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
}

enum AnimationState {
    FEED = 0,
    IDLE = 1,
    PET = 2,
    SLEEP = 3
};

AnimationState currentAnimation = IDLE;
static uint8_t currentFrame = 0;

static const uint16_t FRAME_COUNT = 15;
static const uint16_t FRAME_SIZE = 1024;
static const uint16_t FRAME_TIME_MS = 200;

uint32_t lastFrameTime = 0;

// =====================================================

const char* getAnimationName(AnimationState anim)
{
    switch (anim) {
        case FEED:  return "FEED";
        case IDLE:  return "IDLE";
        case PET:   return "PET";
        case SLEEP: return "SLEEP";
        default:    return "UNKNOWN";
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
// =====================================================

void handleButton()
{
    bool state = digitalRead(BUTTON_PIN);

    if (lastButtonState == HIGH && state == LOW) {
        uint32_t now = millis();

        if (now - lastButtonTime > DEBOUNCE_MS) {
            lastButtonTime = now;
            Serial.println("[BUTTON] FEED");
            feedRequested = true;
            setAnimation(FEED);
        }
    }

    lastButtonState = state;
}

// =====================================================
// LUZ = DORMIR
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
// DISTANCIA = CARICIA
// Si algo pasa por delante a < 5 cm -> PET
// Y PET se deja terminar completa
// =====================================================

void handleDistanceSensor()
{
    uint32_t now = millis();

    if (now - lastDistanceReadTime < DISTANCE_READ_INTERVAL_MS) return;

    lastDistanceReadTime = now;

    float distance = readDistanceCM();
    bool petDetected = (distance > 0.0f && distance <= PET_DISTANCE_CM);

    if (distance > 0.0f) {
        Serial.print("[DIST] ");
        Serial.print(distance);
        Serial.println(" cm");
    } else {
        Serial.println("[DIST] sin lectura");
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
        Serial.println("[DIST] Cerca -> PET");
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

    if (isDark) {
        currentAnimation = SLEEP;
    } else {
        currentAnimation = IDLE;
    }

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
                if (currentAnimation == FEED && feedRequested) {
                    feedRequested = false;
                    setAnimation(IDLE);
                    return;
                }

                if (currentAnimation == PET) {
                    setAnimation(IDLE);
                    return;
                }

                if (currentAnimation == SLEEP) {
                    if (isDark) {
                        currentFrame = SLEEP_HOLD_FRAME;
                        sleepHoldFrame = true;
                    } else {
                        setAnimation(IDLE);
                        return;
                    }
                }

                currentFrame = 0;
            }
        }

        refreshCurrentFrame();
    }
}