# GotchiLab_

Una versión propia creada en **MediaLab_** del clásico Tamagotchi. Está pensada para poder enseñar a niños y jóvenes electrónica básica, sensores, respuestas y reacciones de una forma gráfica, interactiva y educativa.

La mascota es un pequeño pingüino animado que vive dentro de una pantalla OLED SSD1306 y que reacciona tanto a interacciones del usuario (pulsador, caricias táctiles, pin de silencio) como a las condiciones del entorno físico (luminosidad ambiente y niveles de $CO_2$).

Este proyecto se utiliza principalmente en talleres formativos de MediaLab_, donde los participantes montan el hardware en protoboard y cargan el firmware en un microcontrolador **ESP32 DevKit v1**.

---

# Autor

Proyecto creado por:

**José Escobedo Vázquez**  
Integrante de MediaLab_

---

# Descripción del Proyecto

GotchiLab_ es una mascota virtual interactiva que integra una **Máquina de Estados Finita (FSM)**, un **motor de renderizado monocromático con offsets**, retroalimentación de **audio polifónico/tonos mediante PWM (LEDC)** con pin de silencio rápido (`MUTE_PIN`), y un sistema completo de **constantes vitales en segundo plano (modo inmersivo)** con causas de muerte en español y cálculo de puntuación final.

El ciclo de vida comienza con un huevo:
- **Con sensor táctil** $\rightarrow$ eclosiona al acariciar o tocar el sensor.
- **Sin sensor táctil** $\rightarrow$ eclosión automática tras unos segundos configurables (`AUTO_HATCH_DELAY_MS`).

---

# Interacciones y Dinámicas del Juego

| Interacción | Acción en el Gotchi | Efecto en la Lógica de Supervivencia |
| :--- | :--- | :--- |
| **Tocar el sensor touch (en huevo)** | El huevo eclosiona y nace el pingüino (`BIRTH`). | Inicializa los cronómetros vitales de juego. |
| **Pulsar el botón** | El pingüino come (`FEED`). | Satisface el hambre y resetea el contador de inanición (3 min). |
| **Tocar el sensor touch (vivo)** | El pingüino recibe una caricia (`PET`). | Aumenta el nivel de felicidad interna. |
| **Oscuridad (tapar LDR)** | El pingüino se duerme (`SLEEP`). | Recupera energía progresivamente mientras descansa. |
| **Volver a iluminar** | El pingüino despierta y vuelve al reposo. | Conserva la energía acumulada según el tiempo que durmió. |
| **$CO_2$ alto ($\ge 1600$ ppm)** | Se enferma (`TRANSITION_TO_UNHEALTHY`). | Disminuye su salud respiratoria y acumula tiempo de asfixia. |
| **$CO_2$ normal ($\le 1100$ ppm)** | Se recupera sanando (`TRANSITION_TO_HEALTHY`). | Recupera salud respiratoria de forma acelerada (3x). |
| **Puentear GPIO 27 a GND** | Activa / Desactiva el sonido (*Mute Toggle*). | Silencia por completo o restaura el volumen con un *bip*. |
| **Pulsar spam de comida** | El pingüino explota (`POP`) y fallece. | Muerte inmediata por sobrealimentación. |
| **Falta de comida (3 min)** | Muere por inanición. | Activa la marcha fúnebre y la pantalla de Game Over. |
| **Exceso de $CO_2$ prolongado (60s)**| Muere por intoxicación / asfixia. | Activa la marcha fúnebre y la pantalla de Game Over. |
| **Privación de sueño continua** | Muere por agotamiento extremo. | Activa la marcha fúnebre y la pantalla de Game Over. |

---

# Modo de Juego Inmersivo (Estados en Segundo Plano)

Para que el jugador deba prestar atención continua a la mascota y cuidarla de forma intuitiva, **la barra superior de estado se encuentra oculta de la pantalla**:
- La pantalla OLED de 128x64 se dedica **íntegramente a las animaciones** del pingüino a pantalla completa.
- **Toda la lógica de supervivencia sigue ejecutándose en segundo plano**:
  1. **Hambre e Inanición**: Debes alimentarlo periódicamente con el botón; si pasa 3 minutos continuos sin comer, morirá de inanición. Si lo sobrealimentas (spam), explotará (`POP`).
  2. **Felicidad y Afecto**: Necesita caricias táctiles frecuentes para mantener su alegría de fondo.
  3. **Salud Respiratoria ($CO_2$)**: Inicia a tope (salud máxima a 400 ppm). Si el sensor detecta aire viciado continuo, enfermará y podrá morir por asfixia si no se ventila a tiempo. Al ventilarse, recupera su salud rápidamente.
  4. **Sueño y Descanso**: Si se deja mucho tiempo con luz sin descansar, acumulará fatiga de fondo hasta morir por agotamiento. Al apagar la luz / tapar el LDR, recupera energía de forma progresiva.
- *(Opcional)*: Si en algún momento deseas volver a ver los medidores en pantalla, puedes cambiar `#define SHOW_STATS_OVERLAY 1` en `src/config/config.h`.

---

# Función de Silencio (Mute Toggle en GPIO 27)

El sistema incluye una función de hardware para silenciar al pingüino sin necesidad de desconectar el buzzer:
- **Pin asignado**: `GPIO 27` (configurado como `INPUT_PULLUP`).
- **Uso**: Al puentear un cable o pulsador entre **GPIO 27** y **GND**, el sonido se apaga de inmediato.
- **Restauración**: Al volver a puentearlo a **GND**, el sonido se reactiva y emite un breve tono confirmatorio.

---

# Pantalla de Muerte, Game Over y Puntuación

Cuando se produce el deceso por cualquiera de las condiciones, el microcontrolador reproduce una marcha fúnebre mediante PWM (si no está muteado) y presenta la pantalla final durante 9 segundos antes de reiniciar el huevo:

- **Causa de Muerte**:
  - `INANICION 3m`: Si estuvo más de 180 s sin comer.
  - `EXPLOTO (POP)!`: Si se sobrealimentó con pulsaciones rápidas.
  - `ASFIXIA (CO2)`: Si acumuló más de 60 s en ambiente tóxico sin ventilar.
  - `AGOTAMIENTO`: Si estuvo despierto sin dormir más del tiempo límite (2.5 min).
- **Tiempo de Supervivencia**: Segundos exactos que se mantuvo vivo tras eclosionar.
- **Cuidados Registrados**: Conteo total de caricias y alimentaciones exitosas.
- **Puntuación Final**:
  $$\text{Puntuación} = (\text{Segundos vivos} \times 10) + (\text{Comidas} \times 15) + (\text{Caricias} \times 20) - \text{Penalizaciones}$$

---

# Características del Sistema y Estados (FSM)

| Estado | Descripción |
| :--- | :--- |
| `IDLE_EGG` | Estado inicial en reposo dentro del cascarón |
| `BIRTH` | Animación de nacimiento (bloqueante) |
| `IDLE` | Reposo normal y saludable |
| `IDLE_UNHEALTHY` | Estado enfermo por concentración de $CO_2$ |
| `TRANSITION_TO_UNHEALTHY` | Transición de sano a enfermo (bloqueante) |
| `TRANSITION_TO_HEALTHY` | Recuperación a sano (animación invertida, bloqueante) |
| `FEED` | Animación de comer (bloqueante) |
| `PET` | Animación de recibir caricia (bloqueante) |
| `SLEEP` | Dormir (pausado en frame 10 mientras siga oscuro) |
| `POP` | Animación de explosión por sobrealimentación |
| `DEAD` | Pantalla de Game Over con estadísticas y sonido fúnebre |

---

# Hardware

### Componentes Necesarios

| Componente | Cantidad | Descripción |
| :--- | :--- | :--- |
| **ESP32 DevKit v1** | 1 | Microcontrolador principal (30 o 36 pines) |
| **Pantalla OLED SSD1306** | 1 | Display monocromo 128x64 I2C (dirección 0x3C) |
| **Sensor de $CO_2$ Sensirion SCD30** | 1 | Sensor NDIR óptico I2C de alta precisión |
| **Sensor Touch Capacitivo** | 1 | Módulo TTP223 digital |
| **Fotoresistencia (LDR)** | 1 | Sensor de luz en divisor de tensión con resistencia 10kΩ |
| **Resistencia 10kΩ** | 1 | Pull-down para el sensor LDR |
| **Pulsador** | 1 | Push button normalmente abierto |
| **Buzzer pasivo** | 1 | Buzzer piezoeléctrico accionado por PWM |
| **Protoboard y Cables** | varios | Montaje en placa de pruebas y jumpers Dupont |

### Configuración de Pines

| Componente | Pin ESP32 | Modo / Configuración |
| :--- | :--- | :--- |
| **OLED SDA** | GPIO 22 | I2C Data (compartido con SCD30) |
| **OLED SCL** | GPIO 21 | I2C Clock (compartido con SCD30) |
| **SCD30 SDA** | GPIO 22 | I2C Data (compartido con OLED) |
| **SCD30 SCL** | GPIO 21 | I2C Clock (compartido con OLED) |
| **Pulsador** | GPIO 33 | `INPUT_PULLUP` (pulsación activa a GND) |
| **Sensor Touch** | GPIO 14 | `INPUT` digital (HIGH al tocar) |
| **Sensor Luz (LDR)** | GPIO 34 | `INPUT` analógico (ADC) |
| **Buzzer** | GPIO 26 | Salida LEDC PWM (Canal 0, Resolución 8 bits) |
| **Mute (Silencio)** | GPIO 27 | `INPUT_PULLUP` (Puentear a GND alterna entre silencio y sonido) |

---

# Modularidad y Sensores Opcionales

El firmware es **100% tolerante a la ausencia de hardware**. Mediante directivas de precompilación en `src/config/config.h`, se puede desconectar cualquier componente:

```cpp
#define USE_BUTTON_SENSOR       1  // 0: Sin botón ni muerte por inanición
#define USE_TOUCH_SENSOR        1  // 0: Nace solo, sin caricias ni decaimiento de afecto
#define USE_LIGHT_SENSOR        1  // 0: Siempre despierto, sin muerte por insomnio
#define USE_CO2_SENSOR          1  // 0: Siempre sano a 400 ppm, sin asfixia
#define USE_BUZZER              1  // 0: Silencio total
#define SHOW_STATS_OVERLAY      0  // 0: Modo inmersivo pantalla limpia, 1: HUD visible
```

> **Garantía Senior**: Si un sensor está en `0`, sus reglas de muerte quedan completamente deshabilitadas para que el juego nunca penalice al usuario por sensores ausentes.

---

# Software y Compilación

Firmware desarrollado en C++ bajo **PlatformIO** (Framework Arduino para ESP32).

### Librerías Requeridas (`platformio.ini`):
- `adafruit/Adafruit GFX Library`
- `adafruit/Adafruit SSD1306`
- `sparkFun/SparkFun SCD30 Arduino Library`
- `sparkFun/SparkFun SCD4x Arduino Library`
- `Wire`

### Comandos de Compilación y Carga:
```bash
# Compilar proyecto
pio run

# Cargar al ESP32 (especificando puerto si es necesario)
pio run -t upload --upload-port COM12

# Monitor serie
pio device monitor -b 115200
```

---

# Estructura del Repositorio

```text
GotchiLab_/
├── code/
│   ├── platformio.ini              # Configuración del entorno y dependencias
│   ├── agents.md                   # Especificación de roles y arquitectura de agentes
│   └── src/
│       ├── main.cpp                # Firmware principal, FSM, audio, mute y Game Over
│       ├── config/
│       │   └── config.h            # Feature flags, pines, umbrales y tiempos de juego
│       ├── sensors/
│       │   ├── sensors.h           # Declaración del subsistema de sensores
│       │   └── sensors.cpp         # Lógica SCD30 con descarte de anomalías y botones
│       └── animations/             # Arrays de animación monocromáticos en Flash (15 frames)
├── Esquematico/                    # Esquemas de cableado y circuitos (Fritzing, PDF, PNG)
├── VideoToCarray/                  # Herramienta de conversión de vídeo MP4 a arrays C
│   └── mp4_a_c_array_v2.py         # Script OpenCV para generar animaciones
├── prompt.md                       # Especificación completa, prompt de regeneración y diagrama de contexto
├── GotchiLab_.pdf                  # Guía didáctica para talleres educativos
└── README.md                       # Este archivo
```
