# Diseño técnico roguelike (MVP)

## Objetivo

Tener un modo roguelike reproducible por semilla dentro de `pret/pokeemerald`, con:

- Pisos de 3-5 nodos.
- Tipos de nodo: `WILD`, `TRAINER`, `ELITE`, `SHOP`, `REST`, `EVENT`, `BOSS`.
- Escalado simple de peligro y recompensa.

## Reglas de generación (MVP)

1. RNG LCG de 32 bits (`1664525 * state + 1013904223`).
2. Cada piso deriva su RNG desde `seed` + `floorIndex` (paridad entre C y Python).
3. Piso 1 sin `ELITE`.
4. Cada 4 pisos se favorece `SHOP/REST`.
5. Último nodo del último piso = `BOSS`.
6. Semilla fija => salida fija (determinismo).

## Contrato C mínimo

```c
void Rogue_InitRun(RogueRunState *state, u32 seed, u16 totalFloors);
void Rogue_GenerateFloor(const RogueRunState *state, RogueFloor *outFloor, u16 floorIndex);
```

Implementado en:

- `integration/pokeemerald/include/roguelike.h`
- `integration/pokeemerald/src/roguelike.c`

## Plan de integración real en pokeemerald

1. Copiar archivos con `scripts/bootstrap_pokeemerald.sh`.
2. Crear opción de menú “Roguelike Run”.
3. Guardar `seed`, `currentFloor` y `totalFloors` en saveblock.
4. Resolver cada nodo como script/evento del engine.

## Criterios de aceptación MVP

- Se puede iniciar run con seed manual.
- El mismo seed produce el mismo piso.
- Último piso termina en nodo `BOSS`.
- El juego puede continuar al siguiente piso sin romper flujo base.
