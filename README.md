# GotchiLab_

Una versión propia creada en MediaLab_ del clásico tamagotchi. Está pensada para poder enseñar a los niños electrónica básica, sensores, respuestas y reacciones de una forma gráfica e interactiva.

La mascota es un pequeño pingüino animado que vive dentro de una pantalla OLED y que reacciona a distintas interacciones del usuario y también a las condiciones del entorno (como el CO₂).

Este proyecto se utiliza principalmente en talleres educativos de MediaLab_, donde los participantes montan el hardware y cargan el firmware en un ESP32.

---

# Autor

Proyecto creado por:

José Escobedo Vázquez  
Integrante de MediaLab_

---

# Descripción del proyecto

GotchiLab es una mascota electrónica interactiva basada en un ESP32, una pantalla OLED y varios sensores.

El sistema muestra animaciones del pingüino y responde a diferentes acciones del usuario.

El ciclo de vida comienza con un huevo:

- Si hay sensor de distancia → nace al detectar presencia
- Si NO hay sensor → nace automáticamente tras unos segundos

---

# Interacciones disponibles

| Interacción | Acción |
|-------------|------|
| Detectar presencia cerca del sensor | El huevo se abre y nace el pingüino |
| Sin sensor de distancia | Nacimiento automático |
| Pulsar el botón | El pingüino come |
| Pasar la mano cerca | El pingüino recibe una caricia |
| Oscuridad | El pingüino se duerme |
| Vuelve la luz | El pingüino se despierta |
| CO₂ alto | El pingüino se pone enfermo |
| CO₂ normal | El pingüino se recupera |
| Pulsar muchas veces el botón | El pingüino explota y el juego reinicia |
| Sonidos | Expresa emociones |

---

# Características del sistema

El comportamiento del pingüino se basa en diferentes estados:

| Estado | Descripción |
|------|-------------|
| IDLE_EGG | Estado inicial, el pingüino está dentro del huevo |
| BIRTH | Animación de nacimiento |
| IDLE | Estado normal (sano) |
| IDLE_UNHEALTHY | Estado enfermo por CO₂ |
| TRANSITION_TO_UNHEALTHY | Transición a enfermo |
| TRANSITION_TO_HEALTHY | Transición a sano (animación invertida) |
| FEED | Animación de comer |
| PET | Animación de caricia |
| SLEEP | Animación de dormir |
| POP | El pingüino explota |

---

# Reglas de comportamiento

## Nacimiento

- El sistema inicia siempre en IDLE_EGG
- Con sensor → nace al detectar presencia
- Sin sensor → nace automáticamente
- Después pasa a:
  - IDLE (si CO₂ normal)
  - IDLE_UNHEALTHY (si CO₂ alto)
  - SLEEP (si está oscuro)

---

## Alimentación

- Pulsar el botón activa FEED
- La animación no se interrumpe
- Al terminar vuelve al estado base (sano o enfermo)

---

## Caricias

- Distancia < 5 cm activa PET
- La animación siempre se completa
- Luego vuelve al estado base

---

## Sueño

- Oscuridad → SLEEP
- Se queda en un frame fijo mientras siga oscuro
- Al volver la luz → vuelve a IDLE o IDLE_UNHEALTHY

---

## Sistema de CO₂

El sistema usa sensores tipo SCD4x.

### Si el CO₂ sube:

IDLE → TRANSITION_TO_UNHEALTHY → IDLE_UNHEALTHY

### Si el CO₂ baja:

IDLE_UNHEALTHY → TRANSITION_TO_HEALTHY → IDLE

Notas:

- Puede comer, recibir caricias, etc estando enfermo
- Pero volverá a estado enfermo después
- Usa histéresis (dos umbrales) para evitar cambios constantes

---

## Animaciones bloqueantes

Estas animaciones NO se pueden interrumpir:

- BIRTH
- FEED
- PET
- POP
- TRANSITIONS

---

## Sobrealimentación (POP)

Si el botón se pulsa muchas veces seguidas:

- el pingüino explota
- se reproduce animación POP
- se reproduce sonido
- el sistema se reinicia

Flujo:

POP → IDLE_EGG

---

# Hardware

## Componentes necesarios

| Componente | Cantidad |
|------------|---------|
| ESP32 DevKit | 1 |
| Pantalla OLED SSD1306 128x64 | 1 |
| Sensor ultrasónico HC-SR04 | 1 |
| LDR (fotoresistencia) | 1 |
| Sensor CO₂ SCD40 / SCD41 | 1 |
| Resistencia 10kΩ | 1 |
| Resistencias divisor (1kΩ + 2kΩ) | 2 |
| Buzzer pasivo | 1 |
| Botón | 1 |
| Protoboard | 1 |
| Cables Dupont | varios |

---

# Configuración de pines

| Componente | Pin ESP32 |
|-------------|----------|
| OLED SDA | GPIO 22 |
| OLED SCL | GPIO 21 |
| Botón | GPIO 33 |
| HC-SR04 TRIG | GPIO 14 |
| HC-SR04 ECHO | GPIO 27 |
| Sensor de luz | GPIO 34 |
| Buzzer | GPIO 26 |
| SCD4x SDA | GPIO 22 |
| SCD4x SCL | GPIO 21 |

---

# Conexiones

## Pantalla OLED

| OLED | ESP32 |
|------|------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 22 |
| SCL | GPIO 21 |

---

## Sensor CO₂ (SCD4x)

| SCD4x | ESP32 |
|------|------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 22 |
| SCL | GPIO 21 |

Comparte bus I2C con la pantalla.

---

## Botón

| Botón | ESP32 |
|------|------|
| Pin 1 | GPIO 33 |
| Pin 2 | GND |

Configurado con INPUT_PULLUP.

---

## Sensor de distancia (HC-SR04)

| HC-SR04 | ESP32 |
|--------|------|
| VCC | 5V |
| GND | GND |
| TRIG | GPIO 14 |
| ECHO | GPIO 27 |

IMPORTANTE: usar divisor de tensión en ECHO

---

## Sensor de luz (LDR)

Divisor de tensión:

3.3V
 |
[LDR]
 |
 +------ GPIO 34
 |
[10kΩ]
 |
GND

---

## Buzzer

| Buzzer | ESP32 |
|------|------|
| + | GPIO 26 |
| - | GND |

Controlado con PWM (LEDC)

---

# Sensores opcionales

El sistema es robusto:

- Sin distancia → nacimiento automático
- Sin luz → no duerme
- Sin CO₂ → siempre sano
- Sin buzzer → sin sonido

---

# Software

Firmware en C++ (Arduino)

Compatible con:
- PlatformIO
- Arduino IDE

## Librerías necesarias

Adafruit_GFX  
Adafruit_SSD1306  
Wire  
SparkFun SCD4x  

---

# Sistema de animaciones

- 15 frames por animación
- 128x64
- 1024 bytes/frame
- ~3 segundos

## Animaciones incluidas

| Animación | Descripción |
|-----------|-------------|
| penguin_idle_egg | Huevo |
| penguin_birth | Nacimiento |
| penguin_idle | Pingüino sano |
| penguin_idle_unhealthy | Pingüino enfermo |
| transition anim | Cambio de estado |
| penguin_feed | Comer |
| penguin_pet | Caricia |
| penguin_sleep | Dormir |
| penguin_pop | Explosión |

---

# Generación de animaciones desde vídeo

Script Python incluido:

python mp4_a_c_array_v2.py video.mp4 --base-name nombre

Genera:

nombre.c  
nombre.h  

---

# Estructura del proyecto

src/
  main.cpp
  config/
    config.h
  animations/
    *.c
    *.h

---

# Contexto educativo

El proyecto permite enseñar:

- sensores  
- microcontroladores  
- lógica de estados  
- animaciones  
- interacción hardware/software  

Permite:

- montar el circuito  
- cargar firmware  
- modificar animaciones  
- añadir sensores  
- ampliar comportamiento  