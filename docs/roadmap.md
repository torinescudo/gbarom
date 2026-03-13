# Roadmap completo — pokeemerald online + UI Liquid Crystal

## Milestone 0: Baseline (actual)
- [x] Arquitectura de red documentada.
- [x] Relay server funcional (salas 1v1, reenvio de frames).
- [x] Smoke test automatizado del relay.
- [x] API de transporte online (online_link.h/c) con loopback.
- [x] Sistema de temas UI con paletas LC.
- [x] Generador procesal de tiles para marcos de ventana.
- [x] Textbox modernizado (3 lineas, shadow, advance indicator).
- [x] Guia de integracion paso a paso.

## Milestone 1: Compilacion integrada
- [ ] Clonar pokeemerald, copiar modulos, verificar compilacion limpia.
- [ ] Descomentar stubs y conectar a APIs reales del decomp.
- [ ] Validar visualmente en mGBA que las paletas LC se aplican.

## Milestone 2: UI completa Liquid Crystal
- [ ] Exportar tiles de frame como .png y convertir con grit.
- [ ] Reemplazar TODOS los DrawStdWindowFrame del decomp (lista completa en guia).
- [ ] Ajustar la party screen y summary screen a paleta LC.
- [ ] Implementar degradado de fondo en menus (tile pattern 2 tonos).
- [ ] Implementar highlight bar con BLDCNT/BLDALPHA real.
- [ ] Transiciones fade con BLDY funcionando.
- [ ] Efecto scanline sutil en title screen.
- [ ] Beta visual cerrada: screenshots de cada pantalla del juego.

## Milestone 3: Link 1v1 online en emulador
- [ ] Bridge para mGBA: interceptar SIO y conectar con relay.
- [ ] Trade Pokemon completo sin corrupcion via relay.
- [ ] Battle link 1v1 sin desync (lockstep con input delay).
- [ ] Medir y documentar latencia aceptable (target <100ms RTT).

## Milestone 4: Hardware bridge GBA
- [ ] Disenar esquematico MCU (ESP32 o RP2040) + conector link.
- [ ] Firmware: leer SPI del cable, encapsular, enviar por WiFi.
- [ ] Validar timing contra osciloscopio.
- [ ] Trade y battle estables en hardware real.

## Milestone 5: Union Room y multijugador
- [ ] Extender relay a N peers por sala.
- [ ] Adaptar topologia por modo de juego.
- [ ] Implementar sala de union online.
- [ ] Secret Base mixing online.

## Milestone 6: Produccion
- [ ] NAT traversal (STUN/TURN o relay regional).
- [ ] Reconexion automatica con resume de estado.
- [ ] Metricas de sesion (RTT, packet loss, desync rate).
- [ ] Sistema de ban/report para abuso.
- [ ] Documentacion de usuario final.
