# Arquitectura online para pokeemerald en GBA

## Objetivo
Construir una ROM basada en pokeemerald que conserve comportamiento de juego original y permita juego online estable para:
- Emuladores (frontend netplay o plugin de transporte).
- Hardware real GBA (mediante bridge fisico al puerto link).

## Restriccion clave
La GBA no tiene red IP nativa. La compatibilidad online completa requiere una capa de transporte externa:
- Emulador: modulo que traduce eventos de cable link a paquetes de red.
- Hardware real: adaptador (MCU) conectado al puerto link que encapsula tramas y las envia a Internet.

## Arquitectura por capas
1. Capa ROM (pokeemerald)
- Extraer la logica de intercambio/combate para depender de una interfaz de transporte abstracta.
- API sugerida: Init, StartSession, SendFrame, RecvFrame, Tick, IsConnected.

2. Capa bridge (emulador o hardware)
- Convierte eventos seriales del cable en mensajes de red.
- Aplica timestamp de tick y numeracion de secuencia.

3. Relay server
- Emparejamiento por sala.
- Reenvio de tramas entre pares.
- Metadatos de sesion (seed, version de protocolo, tick target).

4. Sincronizacion determinista
- Lockstep por frame con input delay fijo.
- Secuencias monotonicas por peer.
- Mecanismo de resync suave cuando hay perdida/latencia alta.

## Protocolo de red base
Formato: JSON por linea (JSONL) sobre TCP para fase inicial.

Mensajes minimos:
- hello: version del protocolo y del cliente.
- join: sala y lado (0/1).
- start: seed comun de sesion y tick_hz.
- link_frame: trama de cable encapsulada (hex), seq, origen.
- input: opcional para lockstep por entradas.
- ping/pong: salud y RTT.
- leave/error: salida y errores.

## Estrategia anti-desync
- Versionado estricto de protocolo y ROM.
- Hash de estado periodico cada N frames.
- Si el hash diverge:
  - Solicitar snapshot incremental, o
  - Reintento de sesion segun modo (competitivo/social).

## Roadmap
M0. Baseline local
- Compilar pokeemerald vanilla.
- Agregar capa de abstraccion LinkTransport sin cambiar gameplay.

M1. Relay funcional
- Servidor TCP con salas 1v1 y reenvio de tramas.
- Pruebas de humo con clientes simulados.

M2. Integracion en emulador
- Plugin/adaptador para mGBA o BizHawk.
- Trade y battle 1v1 estables con latencia moderada.

M3. Hardware bridge GBA
- Firmware MCU para puerto link (UART/SPI segun diseno).
- Validar timing real con osciloscopio/log serial.

M4. Union Room y multijugador
- Extender de 2 a N peers donde aplique.
- Topologias y reglas por modo.

M5. Robustez productiva
- Reconexion, metricas, observabilidad.
- NAT traversal o relay regional.

## Criterio de "compatibilidad completa online"
- Intercambio Pokemon: estable y sin corrupcion.
- Combate link 1v1: sin desync reproducible.
- Union Room/modos multijugador soportados en su alcance definido.
- Funciona tanto en emulador como en GBA real + bridge.
