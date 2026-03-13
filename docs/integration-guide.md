# Guia de integracion — pokeemerald × online + UI Liquid Crystal

Este documento describe paso a paso como conectar los modulos de este repo
dentro de una copia funcional del decomp pokeemerald.

---

## Prerequisitos

```bash
# Clonar pokeemerald y verificar que compila vanilla
git clone https://github.com/pret/pokeemerald.git
cd pokeemerald
make -j$(nproc)
# Debe generar pokeemerald.gba sin errores.
```

---

## Paso 1 — Copiar archivos de integracion

```bash
# Desde la raiz de gbarom:
cp integration/pokeemerald/include/*.h   pokeemerald/include/
cp integration/pokeemerald/src/*.c       pokeemerald/src/
```

## Paso 2 — Registrar fuentes en el Makefile

Editar `pokeemerald/Makefile` (o `pokeemerald/src/Makefile`) para incluir:
```makefile
C_SRCS += src/online_link.c
C_SRCS += src/ui_theme.c
C_SRCS += src/ui_window_frame.c
C_SRCS += src/ui_textbox.c
```

(En el decomp los `.c` en `src/` se compilan automaticamente;  
 solo asegurate de que no estan excluidos por algun filtro.)

## Paso 3 — Ajustar includes

Los archivos asumen `#include "global.h"` (ya existente en pokeemerald).
Ademas asumen acceso a:
- `palette.h` — `LoadPalette()`
- `gpu_regs.h` — `SetGpuReg()`
- `bg.h` — `LoadBgTiles()`, `CopyBgTilemapBufferToVram()`
- `window.h` — `gWindows[]`, `PutWindowTilemap()`, `CopyWindowToVram()`
- `text.h` — `AddTextPrinter()`

Todos estos ya existen en pokeemerald. Solo hay que descomentar las llamadas
reales dentro de los stubs (marcadas con `// In real build:`).

---

## Paso 4 — Hook de inicializacion

En `src/main.c`, funcion `AgbMain()` (o `CB2_InitMainMenu`), agregar:

```c
#include "ui_theme.h"
#include "online_link.h"

// Dentro de la secuencia de init:
OnlineLink_Init();
UiTheme_Init();
```

## Paso 5 — Reemplazar frames de ventana

Buscar llamadas a `DrawStdWindowFrame` y `DrawDialogFrameWithCustomTileAndPalette`.
Reemplazar por:
```c
UiWindowFrame_Draw(windowId, FRAME_STYLE_STANDARD);
// o
UiWindowFrame_Draw(windowId, FRAME_STYLE_DIALOG);
```

Archivos principales afectados:
- `src/menu.c`
- `src/field_message_box.c`
- `src/battle_message.c`
- `src/start_menu.c`
- `src/party_menu.c`
- `src/bag.c`
- `src/pokemon_summary_screen.c`

## Paso 6 — Reemplazar textbox de dialogo

En `src/field_message_box.c` / `src/script.c`:
```c
// Antes:
DrawDialogueFrame(0, FALSE);
AddTextPrinterParameterized(...);

// Despues:
UiTextbox_Open(0);
UiTextbox_Print(0, text);
```

En el loop de campo (VBlank callback o tarea periodica):
```c
UiTextbox_TickAdvanceIndicator(0);
UiTheme_TickCursorAnim();
UiTheme_TickFade();
```

## Paso 7 — Fade transitions

Reemplazar `BeginNormalPaletteFade(...)` en transiciones de menu:
```c
UiTheme_SetupFadeIn();
// en el loop:
UiTheme_TickFade();
if (!UiTheme_IsFading())
    // continuar
```

## Paso 8 — Highlight de seleccion en menus

En listas (mochila, PC, etc.), despues de dibujar el item seleccionado:
```c
UiTheme_DrawHighlightBar(selectedY, itemHeight);
```

---

## Paso 9 — Modulo Online

### 9a. Compilar con el stub loopback
Ya funciona para pruebas locales (TX se copian a RX sin red).

### 9b. Conectar al relay real (emulador)
Requiere un bridge de emulador que:
1. Intercepte escrituras al registro SIO del cable link.
2. Las envie como `link_frame` al relay server via TCP.
3. Inyecte tramas recibidas de vuelta al registro SIO.

Esto se hace fuera de la ROM (plugin de mGBA/BizHawk), usando
la interfaz definida en `OnlineLink_SendFrame / RecvFrame`.

### 9c. Hardware real
Con un MCU (ESP32, RP2040) conectado al puerto link GBA:
1. Leer datos SPI del cable.
2. Encapsular y enviar al relay por WiFi.
3. Recibir respuesta y reinyectar.

---

## Verificacion

```bash
make -j$(nproc)
# Sin errores ni warnings nuevos.

# Test rapido en mGBA:
mgba pokeemerald.gba
# Verificar que los menus se ven con paleta oscura LC.
```

---

## Cambiar de tema en runtime (opcional)

Agregar en opciones del juego o via tecla secreta:
```c
if (UiTheme_GetCurrent() == UI_THEME_LIQUID_CRYSTAL)
    UiTheme_SetTheme(UI_THEME_VANILLA);
else
    UiTheme_SetTheme(UI_THEME_LIQUID_CRYSTAL);
```

---

## Archivos de este repo relevantes

| Archivo | Funcion |
|---------|---------|
| `include/online_link.h` | API de transporte online |
| `src/online_link.c` | Stub con loopback local |
| `include/ui_theme.h` | Constantes y API del tema LC |
| `src/ui_theme.c` | Paletas, fade, cursor, frames |
| `include/ui_window_frame.h` | API de marco LC |
| `src/ui_window_frame.c` | Generador procesal de tiles |
| `include/ui_textbox.h` | API de textbox modernizado |
| `src/ui_textbox.c` | Implementacion del textbox LC |
| `relay_server/relay.py` | Servidor relay TCP |
| `relay_server/protocol.py` | Codificacion de protocolo |
| `docs/online-architecture.md` | Diseno de red |
| `docs/ui-liquid-crystal-design.md` | Diseno visual |
