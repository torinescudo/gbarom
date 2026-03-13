# Dise\u00f1o Visual: Retro Liquid Crystal para pokeemerald

## Referencia de estilo
Liquid Crystal es un ROM hack de Pokemon Crystal conocido por su UI modernizada
que conserva la estetica pixel retro pero con:
- Paletas oscuras y saturadas (fondo azul/gris oscuro, acentos cian/dorado).
- Marcos de ventana con bordes finos y degradado sutil.
- Cajas de texto con esquinas redondeadas y sombra.
- Tipografia mas compacta y legible.
- Transiciones suaves entre pantallas.

## Paleta base (15-bit GBA)

### Fondo de menu (BG palette slot 13)
| Indice | Color RGB15     | Uso               |
|--------|----------------|-------------------|
| 0      | 0x0000         | Transparente      |
| 1      | 0x2108 (10,8,8)| Fondo oscuro      |
| 2      | 0x318C (12,12,12)| Fondo medio     |
| 3      | 0x4A52 (18,18,18)| Borde exterior  |
| 4      | 0x7BDE (30,30,30)| Texto claro     |
| 5      | 0x7FE0 (0,31,31)| Acento cian       |
| 6      | 0x53FF (31,31,10)| Acento dorado   |
| 7      | 0x001F (31,0,0) | Error/HP baja     |
| 8      | 0x03E0 (0,31,0) | HP alta           |
| 9      | 0x7C00 (0,0,31) | HP media (azul)   |
| 10     | 0x3DEF (15,15,15)| Texto secundario|
| 11     | 0x1084 (4,4,4)  | Sombra de texto   |
| 12-15  | Reservado       | ---               |

### Fondo de dialogo (BG palette slot 14)
Misma base con fondo ligeramente mas claro (0x2529) para contraste.

### Sprite de cursor (OBJ palette slot 15)
Cian brillante con parpadeo suave (alternar 0x7FE0 / 0x6F60 cada 16 frames).

## Window frames
pokeemerald usa window frames de 8x8 tiles definidos en
`graphics/text_window/`. Vamos a reemplazar los 9 tiles (esquinas + bordes + centro):

```
TL  T  TR        TL = esquina superior izquierda con borde 1px + radio 2px
L   C   R        C  = relleno solido del fondo oscuro
BL  B  BR        Todos con sombra 1px inferior/derecha
```

El estilo LC usa bordes de 1px de grosor con una linea interior luminosa
(acento cian) y fondo oscuro uniforme.

## Textbox
- Altura: 3 lineas (48px) en ves de 2, para dar espacio.
- Margen interior: 4px arriba, 6px laterales.
- Indicador de avance: flecha triangular animada (bounce 2px).
- Velocidad de texto default: 1 caracter/frame (rapida pero legible).

## Menus (Bag, Party, Pokemon Summary, etc.)
- Fondo degradado vertical oscuro (tile pattern de 2 tonos).
- Separadores entre items: linea horizontal 1px en color borde.
- Item seleccionado: highlight con barra cian traslucida (blend mode).
- Iconos: bordes reducidos, mas espacio para texto.

## Pantalla de titulo
- Logo con paleta dorada sobre fondo azul profundo.
- Efecto scanline sutil (alternar lineas con alpha reduce).

## Transiciones
- Fade in/out usando BLDCNT/BLDY con rampa de 4 frames.
- Cortinilla horizontal para entrar a combate (estilo gen2 modernizado).

## Implementacion tecnica
Todos los cambios se hacen mediante:
1. **Paletas**: Archivos .pal / constantes C en `ui_theme.h`.
2. **Tiles**: Archivos .png de 4bpp convertidos por `grit` dentro del Makefile.
3. **Logica**: Hooks en las funciones de dibujado de ventana/textbox de pokeemerald.
4. **Tema**: Sistema configurable para aplicar LC o vanilla con un flag.
