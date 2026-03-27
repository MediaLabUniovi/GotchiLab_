## Cómo funciona

El programa funciona como una máquina de estados.  
Eso significa que el personaje siempre está en una animación concreta (idle, comer, dormir, etc.) y cambia según lo que pase.

La variable principal es:
- currentAnimation → guarda el estado actual

Las animaciones se cambian usando:
- setAnimation(...)

## Animaciones

Cada animación:
- tiene varios frames (imágenes)
- cada frame es un array de bytes (bitmap)
- se dibuja en la pantalla con drawFrameRaw()

## Sensores (opcionales)

El código permite usar varios sensores, todos activables/desactivables con defines:

- Botón → dar de comer
- Distancia → detectar mano (caricia / nacimiento)
- Luz → dormir si está oscuro
- CO₂ (SCD4x) → estado “enfermo” si el aire es malo
- Buzzer → sonidos

Si un sensor no está conectado o se desactiva, esa funcionalidad desaparece automáticamente.

## Lógica básica

- Empieza como huevo
- Nace (por sensor o automáticamente)
- Entra en estado normal (idle)
- Cambia según:
  - botón → comer
  - mano → caricia
  - oscuridad → dormir
  - CO₂ alto → estado malo

Si mejora el CO₂:
- vuelve a estado normal usando la animación al revés

## CO₂ (importante)

Se usa histéresis:
- entra en “malo” con CO₂ alto
- solo vuelve a “bien” cuando baja suficiente

Esto evita cambios constantes.

## Animaciones inversas

Las transiciones se reutilizan:
- ida → frames normales
- vuelta → mismos frames pero al revés

## Sonido

Los sonidos son listas de notas:
- frecuencia + duración

Cada animación puede tener su sonido.

## Qué puedes modificar

- Activar sensores (defines)
- Umbrales (luz, distancia, CO₂)
- Velocidad de animación (FRAME_TIME_MS)
- Sonidos
- Añadir nuevas animaciones

## Resumen

- Sistema basado en estados
- Animaciones en arrays
- Sensores controlan el comportamiento
- Todo modular y opcional

Pensado para que sea fácil de ampliar y modificar.