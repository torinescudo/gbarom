# Pokémon Emerald Trashman Roguelike Generativo

Base técnica para convertir `pret/pokeemerald` en una experiencia roguelike generativa por semilla, inspirada en Emerald Trashman.

## Estado actual (honesto)

Este repo **sí es útil**, pero aún está en etapa de integración técnica:

- Ya existe generador determinista en Python y en C (paridad probada).
- Ya existe scaffold para copiar el módulo al árbol de `pokeemerald`.
- **Todavía falta** conectar menú, guardado completo y resolución gameplay de cada nodo dentro del engine.

En otras palabras: hoy resuelve el núcleo procedural; todavía no es una ROM roguelike completa lista para jugar de punta a punta.

## Qué incluye este repo

- `tools/seed_generator.py`: prototipo determinista de rutas/nodos + equipos enemigos adaptados (nivel/IA).
- `integration/pokeemerald/include/roguelike.h`: API C mínima para integrar en engine GBA.
- `integration/pokeemerald/src/roguelike.c`: implementación portable (RNG + generación de piso).
- `scripts/bootstrap_pokeemerald.sh`: clona `https://github.com/pret/pokeemerald` y copia el módulo base.
- `tools/make_ips_patch.py`: crea un parche `.ips` entre ROM base y ROM modificada.
- `scripts/build_ips.sh`: compila `pokeemerald.gba` y genera el `.ips` automáticamente.
- `docs/roguelike_design.md`: diseño y roadmap.

## Integración rápida contra `pret/pokeemerald`

```bash
./scripts/bootstrap_pokeemerald.sh ./pokeemerald
```

Después, en el repo destino:

1. Incluir `roguelike.h` en el flujo donde inicializas partida.
2. Llamar `Rogue_InitRun(...)` y `Rogue_GenerateFloor(...)` para construir el mapa de cada run.
3. Llamar `Rogue_GenerateEnemyTeam(...)` para crear equipo enemigo escalado al nivel/cantidad del equipo del jugador.
4. Llamar `Rogue_BuildBattlePlan(...)` para resolver IA/recompensa/tipo de encuentro por nodo (wild/trainer/elite/boss/support/event).
5. Llamar `Rogue_ResolveNode(...)` para obtener recompensa/efecto del nodo elegido (oro, heal/shop).
6. Gestionar el Hub con `Rogue_HubInit`, `Rogue_HubApplyResolution`, `Rogue_HubUpgradeTeamSize`, `Rogue_HubUpgradePower` (formación/mejora de equipo entre runs).
7. Evaluar progreso de medallas con `Rogue_EvaluateGymLeader(...)` cuando toque líder difícil.
8. Llamar `Rogue_AdvanceRun(...)` y comprobar `Rogue_RunIsFinished(...)` para cerrar run al boss final.
9. Guardar `RogueRunState` y estado de hub en saveblock para retomar la run.
10. Cada nodo ya trae `mapGroup/mapNum` y puedes usar `Rogue_GenerateZoneChoices(...)` para mostrar 2-3 destinos al cambiar de zona.


> Nota: el `Makefile` de `pret/pokeemerald` ya incluye automáticamente `src/*.c` por wildcard.

## Generar parche IPS

Con una ROM base legal que tú ya tengas y un checkout compilable de `pokeemerald`:

```bash
./scripts/build_ips.sh ./pokeemerald ./baserom.gba ./out/roguelike_patch.ips
```

También puedes crear IPS manualmente:

```bash
python3 tools/make_ips_patch.py --base ./baserom.gba --target ./pokeemerald/pokeemerald.gba --output ./out/roguelike_patch.ips
```

## Prototipo en Python

```bash
python3 tools/seed_generator.py --seed 1337 --floors 8 --output run_1337.json
```

## Validación de paridad C/Python

```bash
pytest -q
```

Incluye una prueba que compila un harness C local (`tests/c_harness/rogue_dump.c`) y compara la salida de `integration/pokeemerald/src/roguelike.c` contra `tools/seed_generator.py` para varias semillas/pisos.

## Nota legal

No distribuyas ROMs comerciales ni assets propietarios en el repositorio.
