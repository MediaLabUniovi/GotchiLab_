Una versión propia creada en **MediaLab_** del clásico *tamagotchi*. Está pensada para poder enseñar a los niños **electrónica básica, sensores, respuestas y reacciones** de una forma gráfica e interactiva.

La mascota es un pequeño **pingüino animado** que vive dentro de una pantalla OLED y que reacciona a distintas interacciones del usuario.

Este proyecto se utiliza principalmente en **talleres educativos de MediaLab_**, donde los participantes montan el hardware y cargan el firmware en un ESP32.

---

# Autor

Proyecto creado por:

**José Escobedo Vázquez**  
Integrante de **MediaLab_**

---

# Descripción del proyecto

GotchiLab es una **mascota electrónica interactiva** basada en un **ESP32**, una **pantalla OLED** y varios sensores.

El sistema muestra animaciones del pingüino y responde a diferentes acciones del usuario.

Interacciones disponibles:

| Interacción | Acción |
|-------------|------|
| Pulsar el botón | El pingüino come |
| Pasar la mano cerca | El pingüino recibe una caricia |
| Oscuridad | El pingüino se duerme |
| Vuelve la luz | El pingüino se despierta |
| Sonidos | Expresa emociones |

El objetivo del proyecto es explicar de forma sencilla:

- sensores
- entradas digitales
- entradas analógicas
- microcontroladores
- animaciones en pantalla
- interacción hardware/software

---

# Características del sistema

El comportamiento del pingüino se basa en diferentes estados:

| Estado | Descripción |
|------|-------------|
| IDLE | Estado normal |
| FEED | Animación de comer |
| PET | Animación de caricia |
| SLEEP | Animación de dormir |

Reglas de comportamiento implementadas:

- La animación **PET** siempre se completa una vez iniciada.
- La animación **SLEEP** se detiene en el frame 10 si continúa la oscuridad.
- Cuando vuelve la luz la animación continúa de forma natural.
- La animación **FEED** tiene prioridad hasta finalizar.

---

# Hardware

## Componentes necesarios

| Componente | Cantidad |
|------------|---------|
| ESP32 DevKit | 1 |
| Pantalla OLED SSD1306 128x64 | 1 |
| Sensor ultrasónico HC-SR04 | 1 |
| LDR (fotoresistencia) | 1 |
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

## Botón

Configurado usando `INPUT_PULLUP`.

| Botón | ESP32 |
|------|------|
| Pin 1 | GPIO 33 |
| Pin 2 | GND |

Cuando se pulsa el botón se activa la animación **FEED**.

---

## Sensor de distancia (HC-SR04)

| HC-SR04 | ESP32 |
|--------|------|
| VCC | 5V |
| GND | GND |
| TRIG | GPIO 14 |
| ECHO | GPIO 27 |

⚠ Importante:

El pin **ECHO** del HC-SR04 trabaja a **5V**, lo cual no es seguro para el ESP32.  
Debe usarse un **divisor de tensión**.

Ejemplo:

```
HC-SR04 ECHO
      |
     1kΩ
      |
      +---- GPIO 27
      |
     2kΩ
      |
     GND
```

---

## Sensor de luz (LDR)

Se conecta como divisor de tensión.

```
3.3V
 |
[LDR]
 |
 +------ GPIO 34
 |
[10kΩ]
 |
GND
```

El ESP32 mide el nivel de luz con `analogRead()`.

---

## Buzzer

| Buzzer | ESP32 |
|------|------|
| + | GPIO 26 |
| - | GND |

El buzzer se controla mediante **PWM del ESP32 (LEDC)**.

Produce:

- sonido al comer
- sonido al acariciar
- melodía al dormir



<img width="1861" height="864" alt="image" src="https://github.com/user-attachments/assets/c9793ffc-06e1-47dd-919d-9f5842d177b1" />



---

# Software

El firmware está escrito en **C++ usando el framework Arduino**.

Puede compilarse con:

- PlatformIO
- Arduino IDE

## Librerías necesarias

```
Adafruit_GFX
Adafruit_SSD1306
Wire
```

---

# Sistema de animaciones

Las animaciones se almacenan como arrays de frames.

Cada animación contiene:

- 15 frames
- resolución **128x64**
- **1024 bytes por frame**

Velocidad de reproducción:

```
FRAME_TIME_MS = 200 ms
```

Duración aproximada:

```
3 segundos por animación
```

---

# Generación de animaciones desde vídeo

El repositorio incluye un **script en Python** que permite convertir vídeos en animaciones compatibles con el firmware.

El script:

- redimensiona el vídeo a **128x64**
- convierte cada frame a **bitmap monocromo**
- genera automáticamente dos archivos:

```
NombreDelVideo.c
NombreDelVideo.h
```

Estos archivos contienen los arrays de frames que se usan directamente en el firmware.

---

## Uso del script

1. Colocar el vídeo dentro de la misma carpeta que el script.

2. Abrir una terminal en esa carpeta.

3. Ejecutar:

```
python mp4_a_c_array_v2.py NombreDelVideo.mp4 --base-name NombreDelVideo
```

Esto generará:

```
NombreDelVideo.c
NombreDelVideo.h
```

que podrán integrarse directamente en el proyecto.

---


# Contexto educativo

El proyecto se utiliza en talleres de **MediaLab_** para introducir conceptos como:

- sensores
- microcontroladores
- animaciones en pantallas pequeñas
- programación embebida
- interacción hardware/software

Está diseñado para que los participantes puedan:

- montar el circuito
- cargar el firmware
- modificar animaciones
- experimentar con nuevos comportamientos.
