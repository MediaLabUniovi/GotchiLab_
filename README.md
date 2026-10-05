# GotchiLab_

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-teal.svg)](https://www.arduino.cc/)
[![Toolchain: PlatformIO](https://img.shields.io/badge/Toolchain-PlatformIO-orange.svg)](https://platformio.org/)

**GotchiLab_** es una mascota electrónica interactiva de código abierto diseñada y desarrollada en **MediaLab_** para talleres educativos de tecnología y divulgación **STEAM** (Ciencia, Tecnología, Ingeniería, Arte y Matemáticas).

La criatura virtual es un pingüino animado que habita en una pantalla OLED monocromática de 128x64 píxeles gobernada por un microcontrolador **ESP32 DevKit v1**. El firmware reacciona tanto a estímulos directos del usuario (pulsador de alimentación, caricias táctiles capacitivas, conmutador de sonido) como a magnitudes físicas ambientales (nivel de luminosidad mediante fotorresistencia LDR y concentración de dióxido de carbono $CO_2$ mediante sensor óptico NDIR).

---

## Índice

1. [Objetivos Educativos STEAM](#1-objetivos-educativos-steam)
2. [Arquitectura del Firmware y Ciclo de Vida](#2-arquitectura-del-firmware-y-ciclo-de-vida)
3. [Asignación de Pines (Pinout Unificado)](#3-asignación-de-pines-pinout-unificado)
4. [Ajuste y Calibración Analógica del Divisor LDR](#4-ajuste-y-calibración-analógica-del-divisor-ldr)
5. [Bypass Hardware de CO2: Modo Ferias y Demostraciones](#5-bypass-hardware-de-co2-modo-ferias-y-demostraciones)
6. [Máquina de Estados y Secuencia Gráfica de Muerte](#6-máquina-de-estados-y-secuencia-gráfica-de-muerte)
7. [Ciclo de Vida Autocontenido (Sin Persistencia)](#7-ciclo-de-vida-autocontenido-sin-persistencia)
8. [Guía de Compilación y Carga con PlatformIO](#8-guía-de-compilación-y-carga-con-platformio)
9. [Generación de Binario Unificado y Flasheo Web](#9-generación-de-binario-unificado-y-flasheo-web)
10. [Estructura del Proyecto](#10-estructura-del-proyecto)
11. [Licencia](#11-licencia)

---

## 1. Objetivos Educativos STEAM

El proyecto busca desmitificar el desarrollo de sistemas embebidos mediante una experiencia tangible:
- **Electrónica Analógica y Digital**: Comprensión práctica de divisores resistivos, saturación de sensores LDR, buses de comunicación serie ($I^2C$), transductores piezoeléctricos PWM y sensores capacitivos táctiles.
- **Calidad del Aire y Conciencia Ambiental**: Introducción a la física de gases y sensores infrarrojos no dispersivos (NDIR) con el Sensirion SCD30, relacionando la concentración de $CO_2$ en recintos cerrados con la salud de la mascota.
- **Ingeniería de Software para Sistemas Embebidos**: Implementación de una Máquina de Estados Finita (FSM), eliminación de bloqueos (`delay()`) en favor de contadores no bloqueantes con `millis()`, gestión de buffers gráficos en RAM y modularidad tolerante a ausencias de hardware.

---

## 2. Arquitectura del Firmware y Ciclo de Vida

El sistema opera bajo un bucle cooperativo en tiempo real gobernado por `code/src/main.cpp`:
- **Capa Gráfica**: 15 cuadros monocromáticos de 128x64 píxeles por animación (1024 bytes/cuadro empaquetados en memoria Flash), renderizados a 5 FPS (200 ms por cuadro) con offsets verticales dinámicos.
- **Audio PWM**: Controlador de sonido sobre el canal 0 del generador LEDC del ESP32 con secuencias de notas no bloqueantes y conmutación de silencio por hardware.
- **Monitor de Constantes Vitales**: Evaluación periódica de hambre, afecto, fatiga por vigilia prolongada y toxicidad por $CO_2$.

```mermaid
flowchart TD
    EGG([Huevo IDLE_EGG]) -->|Toque TTP223 / Auto| BIRTH([Nacimiento BIRTH])
    BIRTH --> IDLE([Reposo Saludable IDLE])
    
    IDLE -->|Pulsador| FEED([Comiendo FEED])
    FEED --> IDLE
    
    IDLE -->|Toque TTP223| PET([Caricia PET])
    PET --> IDLE
    
    IDLE -->|Oscuridad LDR| SLEEP([Durmiendo SLEEP])
    SLEEP -->|Luz LDR| IDLE
    
    IDLE -->|CO2 >= 1600 ppm| T_UNHEALTHY([Transicion Enfermo])
    T_UNHEALTHY --> SICK([Reposo Enfermo IDLE_UNHEALTHY])
    SICK -->|CO2 <= 1100 ppm| T_HEALTHY([Transicion Sano])
    T_HEALTHY --> IDLE
    
    IDLE -.->|Inanicion / CO2 / Agotamiento / Spam| POP([Explosion POP - 15 Frames])
    SICK -.->|Inanicion / CO2 / Agotamiento| POP
    
    POP --> DEAD([Pantalla Game Over y Puntuacion])
    DEAD -->|Timeout 9s / Reset Pulsador tras 1.5s| EGG
```

---

## 3. Asignación de Pines (Pinout Unificado)

La arquitectura centraliza la asignación de pines en `code/include/pins_config.h`:

| Periférico / Señal | Pin ESP32 | Modo GPIO | Descripción Técnica |
| :--- | :---: | :---: | :--- |
| **I2C SDA** (OLED & SCD30) | `GPIO 22` | $I^2C$ Data | Bus de datos bidireccional compartido |
| **I2C SCL** (OLED & SCD30) | `GPIO 21` | $I^2C$ Clock | Señal de reloj de bus serie compartido |
| **Pulsador Alimentar** | `GPIO 33` | `INPUT_PULLUP` | Pulsador activo a nivel bajo (GND) |
| **Sensor Táctil (TTP223)** | `GPIO 27` | `INPUT` | Entrada digital activa a nivel alto (VCC) |
| **Sensor de Luz (LDR)** | `GPIO 34` | `ADC1_CH6` | Entrada analógica (solo lectura, sin pull-up) |
| **Buzzer Piezoeléctrico** | `GPIO 26` | Salida LEDC | Modulación PWM (Canal 0, 8 bits) |
| **Conmutador Silencio (Mute)** | `GPIO 32` | `INPUT_PULLUP` | Puente a GND conmuta entre sonido y silencio |
| **Modo Ferias (Bypass CO2)** | `GPIO 25` | `INPUT_PULLUP` | Jumper a GND: anula inicialización de $CO_2$ |

---

## 4. Ajuste y Calibración Analógica del Divisor LDR

### Topología del Circuito
El sensor de luz implementa un divisor de tensión resistivo entre la línea de 3.3V y masa:

```text
       3.3V (VCC)
           │
         ┌─┴─┐
         │LDR│ (Fotorresistencia)
         └─┬─┘
           ├───────> GPIO 34 (ADC1_CH6 del ESP32)
         ┌─┴─┐
         │10k│ (Resistencia Pull-Down de 10 kΩ)
         └─┬─┘
           │
          GND
```

### Ecuación de Transferencia
$$V_{out} = V_{CC} \cdot \left( \frac{R_{pull}}{R_{LDR} + R_{pull}} \right)$$

$$\text{Cuentas ADC (12 bits)} = \left( \frac{V_{out}}{3.3\,\text{V}} \right) \cdot 4095$$

- **En luz ambiente**: La resistencia $R_{LDR}$ cae a valores bajos (~1 kΩ a 5 kΩ), por lo que $V_{out} \approx 2.75\,\text{V}$ (ADC $\approx 3412$).
- **En oscuridad (tapado)**: La resistencia $R_{LDR}$ aumenta drásticamente (> 50 kΩ a 100+ kΩ), por lo que $V_{out}$ cae por debajo de $1.0\,\text{V}$ (ADC < 1240).

### Calibración Paso a Paso con Multímetro
1. Configura el multímetro en escala de tensión continua (**DC Voltios**, rango 20V o 2V).
2. Conecta la sonda negra a **GND** y la sonda roja al punto medio del divisor (**GPIO 34**).
3. Mide la tensión en condiciones de iluminación de trabajo ($V_{luz} \approx 2.5\,\text{V} - 3.0\,\text{V}$).
4. Cubre completamente la fotorresistencia con el dedo y anota la tensión mínima ($V_{oscuro} \approx 0.5\,\text{V} - 1.5\,\text{V}$).
5. Establece el punto de conmutación en el valor medio:
   $$V_{umbral} = \frac{V_{luz} + V_{oscuro}}{2}$$
   $$\text{LDR\_DARK\_THRESHOLD} = \left(\frac{V_{umbral}}{3.3\,\text{V}}\right) \cdot 4095$$
6. Actualiza la constante `LDR_DARK_THRESHOLD` en `code/include/config.h` (valor predeterminado: **2856**, correspondiente a ~2.30V).

### Telemetría Serie de Depuración
Para verificar lecturas en tiempo real sin cálculos manuales, activa la directiva en `code/include/config.h`:
```cpp
#define DEBUG_LDR_CALIBRATION 1
```
Abre la consola serie a **115200 baudios** para observar el volcado continuo:
```text
[LDR DEBUG] ADC Raw: 3120/4095 | Voltaje: 2.512 V | Umbral: 2856 | Estado: ILUMINADO (DESPIERTO)
[LDR DEBUG] ADC Raw: 1150/4095 | Voltaje: 0.926 V | Umbral: 2856 | Estado: OSCURO (DORMIR)
```

---

## 5. Bypass Hardware de CO2: Modo Ferias y Demostraciones

En eventos masivos, ferias de ciencias o aulas cerradas, la concentración de $CO_2$ suele superar con facilidad los 1600 ppm, provocando que la mascota enferme continuamente o muera por asfixia en menos de un minuto durante las explicaciones.

Para solventarlo sin recompilar el código:
1. **Jumper Físico**: Se define `PIN_FAIR_MODE` en `GPIO 25` configurado con `INPUT_PULLUP`.
2. **Detección en Arranque**: Durante el `setup()`, el microcontrolador verifica el estado de `GPIO 25`:
   - Si está conectado a **GND** mediante un jumper o cable Dupont:
     - Se omite la inicialización de la librería del sensor SCD30 y el tráfico por el bus $I^2C$.
     - La variable interna `co2SensorEnabled` pasa a `false`.
     - Las lecturas devuelven un valor nominal limpio y constante de **400 ppm**.
     - La máquina de estados ignora totalmente las penalizaciones y alertas respiratorias.
   - Si el jumper está abierto (flotante / HIGH): El sensor NDIR opera con normalidad.

### Exclusión Completa en Compilación
Si se construye una versión económica de la mascota sin el sensor Sensirion SCD30 instalado físicamente, puede excluirse íntegramente del binario definiendo en `code/include/config.h`:
```cpp
#define USE_CO2_SENSOR 0
```

---

## 6. Máquina de Estados y Secuencia Gráfica de Muerte

Para ofrecer un dramatismo lúdico intuitivo y eliminar transiciones abruptas:

1. **Detección de Fallecimiento**: Cuando se cumple cualquier condición crítica (inanición, asfixia por $CO_2$, privación de sueño o sobrealimentación), la función `triggerDeath(reason)` captura la causa y el timestamp.
2. **Ejecución Obligatoria de Animación "POP"**:
   - El sistema entra en el estado `POP` reproduciendo los 15 cuadros de explosión/desvanecimiento (`penguin_pop_anim`) a 200 ms por cuadro (3 segundos en total).
   - Se silencia cualquier sonido ambiental y se reproduce el efecto acústico de explosión.
3. **Transición a Pantalla de Game Over**:
   - Una vez finalizado el cuadro 14 de `POP`, la máquina conmuta formalmente a `DEAD`.
   - Se activa la marcha fúnebre mediante PWM.
   - Se limpia el buffer de vídeo y se imprime la esquela con:
     * Causa específica de defunción.
     * Segundos totales vividos tras la eclosión.
     * Cuidados totales (caricias táctiles y alimentaciones con éxito).
     * Puntuación matemática final.
4. **Buffer Libre de Parpadeos (*Flicker-Free*)**:
   - Todas las operaciones de renderizado se realizan exclusivamente en la memoria RAM del ESP32 (buffer de 1024 bytes de Adafruit_SSD1306).
   - La pantalla solo se actualiza en bloque mediante una única transacción $I^2C$ (`display.display()`), garantizando ausencia total de artefactos visuales.

---

## 7. Ciclo de Vida Autocontenido (Sin Persistencia)

Siguiendo la especificación de diseño educativo:
- **Cero Persistencia**: Se eliminan todas las dependencias de memoria no volátil (`Preferences.h`, `EEPROM.h`, NVS flash).
- **Reinicio Integral**: Al expirar el tiempo de la pantalla de muerte (9 segundos) o al presionar cualquier botón tras 1.5 segundos de gracia, la función `resetToEggState()` restablece de forma absoluta e incondicional todos los temporizadores, acumuladores de daño, banderas de estado, contadores de spam y variables de felicidad a sus valores base.
- Cada partida es un experimento nuevo, autónomo e independiente.

---

## 8. Guía de Compilación y Carga con PlatformIO

### Requisitos Previos
- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/index.html) o extensión oficial de PlatformIO para [Visual Studio Code](https://code.visualstudio.com/).
- Cable USB de datos conectado al ESP32.

### Compilación desde CLI

Accede al directorio `code/`:
```bash
cd code
```

```bash
# Compilar el proyecto completo
pio run
```

### Carga al Microcontrolador

```bash
# Carga automática en el primer puerto detectado:
pio run -t upload

# O especificando el puerto COM (Windows) o /dev/ttyUSB* (Linux/Mac):
pio run -t upload --upload-port COM3
```

### Monitor Serie

```bash
pio device monitor -b 115200
```

---

## 9. Generación de Binario Unificado y Flasheo Web

Para talleres masivos donde los alumnos no disponen del entorno de desarrollo ni de compiladores, se puede fusionar el bootloader, las tablas de partición y el firmware en un único archivo binario (`firmware_merged.bin`) para grabarlo directamente desde el navegador web o con `esptool`.

### 1. Generar el Binario Fusionado con esptool
Tras compilar con PlatformIO (`pio run`), ejecuta la siguiente orden desde el directorio `code/`:

```bash
python -m esptool --chip esp32 merge_bin -o firmware_merged.bin \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 .pio/build/esp32dev/bootloader.bin \
  0x8000 .pio/build/esp32dev/partitions.bin \
  0x10000 .pio/build/esp32dev/firmware.bin
```

> [!TIP]
> Si utilizas la herramienta interna de PlatformIO, la ruta suele ser:
> `~/.platformio/packages/tool-esptoolpy/esptool.py`

### 2. Flashear sin Compilar mediante Línea de Comandos
Para programar un microcontrolador virgen con el archivo generado:

```bash
python -m esptool --chip esp32 --port COM3 --baud 460800 write_flash 0x0 firmware_merged.bin
```

### 3. Flasheo Web (ESP Web Tools / Navegador Web)
1. Conecta el ESP32 al ordenador mediante cable USB.
2. Abre Google Chrome o Microsoft Edge y visita una plataforma de programación web compatible con WebSerial (como [ESP Web Tools](https://esphome.github.io/esp-web-tools/) o [Adafruit WebSerial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/)).
3. Selecciona **Connect** y elige el puerto serie del ESP32.
4. Carga el archivo `firmware_merged.bin` en el offset `0x0000` (o `0x0`).
5. Pulsa **Program / Flash**. El firmware se cargará en segundos sin requerir instalación de Python, drivers ni software de compilación.

---

## 10. Estructura del Proyecto

```text
GotchiLab_/
├── LICENSE                         # Licencia de software de código abierto MIT
├── README.md                       # Manual técnico y guía de ingeniería
├── prompt.md                       # Especificación maestra del sistema
├── GotchiLab_.pdf                  # Guía didáctica para talleres presenciales
├── Esquematico/                    # Esquemas Fritzing (.fzz), partes (.fzpz), PDF y PNG
├── PlacaSTL/                       # Archivos de fabricación 3D para chasis y PCB
├── VideoToCarray/                  # Pipeline Python/OpenCV para conversión de vídeo a C
└── code/
    ├── platformio.ini              # Configuración PlatformIO (ESP32 DevKit v1)
    ├── include/
    │   ├── pins_config.h           # Centralización de GPIOs y asignación de pines
    │   └── config.h                # Feature flags, umbrales y tiempos de supervivencia
    └── src/
        ├── main.cpp                # FSM principal, gestión gráfica, audio y loop vital
        ├── config/
        │   └── config.h            # Reenvío de compatibilidad hacia include/config.h
        ├── sensors/
        │   ├── sensors.h           # Declaración del subsistema de sensores
        │   └── sensors.cpp         # SCD30 con bypass por jumper de ferias y botón
        └── animations/             # Cuadros de animación monocromáticos en Flash (15 frames)
```

---

## 11. Licencia

Este proyecto está distribuido bajo la licencia de código abierto **MIT**. Consulta el archivo [LICENSE](LICENSE) para más detalles.

Desarrollado con pasión para la comunidad maker y educativa por **José Escobedo Vázquez** en **MediaLab_**.
