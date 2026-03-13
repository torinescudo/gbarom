# Relay Server (fase inicial)

Servidor relay para transportar trafico de cable link de pokeemerald sobre TCP.

## Ejecutar

```bash
cd relay_server
python3 relay.py --host 0.0.0.0 --port 8765 --tick-hz 60
```

## Protocolo rapido
Mensajes JSON por linea.

Handshake:
```json
{"type":"hello","version":"1","name":"player-a"}
{"type":"join","room":"room-001","side":0}
```

Trama de link:
```json
{"type":"link_frame","room":"room-001","seq":120,"payload":"a1b203ff"}
```

El relay reenvia agregando `from` con el lado origen.

## Alcance actual
- Salas 1v1.
- Versionado de protocolo.
- Start de sesion al tener 2 jugadores.
- Reenvio de `link_frame`, `input` y `state_hash`.

## Siguiente paso
Integrar bridge de emulador/hardware para convertir eventos reales de cable link al protocolo de esta carpeta.
