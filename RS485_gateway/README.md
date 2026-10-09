# KinCony A2 USB-RS485-Gateway

`kincony_a2_usb_rs485.yaml` verbindet den eingebauten USB-UART des
klassischen KinCony A2 mit dessen RS485-Anschluss. Der PC spricht das
Modbus-RTU-Geraet direkt ueber den USB-COM-Port an. Es gibt keine
Modbus-TCP-Konvertierung, Registerauswertung oder CRC-Aenderung.

## Anschluesse

| Verbindung | ESP32 TX | ESP32 RX |
| --- | --- | --- |
| Eingebauter USB-UART | GPIO1 | GPIO3 |
| RS485 | GPIO32 | GPIO35 |

Die Konfiguration ist fuer das klassische A2 mit automatischer
RS485-Richtungsumschaltung vorgesehen. Sie ist nicht fuer A2-Revisionen
mit anderer Pinbelegung oder extern erforderlichem DE/RE-Signal gedacht.

USB mit dem PC verbinden und das Modbus-Geraet an die RS485-Klemmen A/B
anschliessen. Board und Geraet gemaess Hersteller versorgen; USB nicht
ungeprueft als ausreichende Boardversorgung voraussetzen. Bezugspotential,
Busabschluss und Bias gemaess Busaufbau und Herstellerangaben vorsehen.
Nur ein Modbus-Master darf auf diesem Bus aktiv sein.

## Serielle Einstellungen

Standard: **9600 Baud, 8 Datenbits, keine Paritaet, 1 Stopbit (8N1)**.
Die Substitutionen am Anfang der YAML muessen zum Modbus-Geraet passen:

- `serial_baud_rate`: beispielsweise `"9600"` oder `"19200"`.
- `serial_parity`: `"NONE"`, `"EVEN"` oder `"ODD"`.
- `serial_stop_bits`: `"1"` oder `"2"`.

Die PC-Software muss dieselben Einstellungen verwenden. Aenderungen am
COM-Port werden vom USB-UART nicht als Konfigurationswechsel an ESPHome
weitergereicht; nach YAML-Aenderungen neu kompilieren und flashen.

Die Bridge buendelt empfangene Bytes bis zu einer Pause von 5 ms oder
256 Bytes und sendet sie unveraendert an die andere UART. Das entspricht
der maximalen Modbus-RTU-Telegrammgroesse. ESPHome-Scheduling und Pufferung
verursachen zusaetzliche Latenz; am PC zunaechst 1000 ms Antworttimeout
waehlen. Dies ist keine zeitlich garantierte Echtzeit-Bridge, insbesondere
nicht fuer hohe Baudraten oder Protokolle mit strikten Timinganforderungen.

## Flashen und Betrieb

Aus dem Repository-Hauptverzeichnis:

```powershell
esphome config RS485_gateway\kincony_a2_usb_rs485.yaml
esphome compile RS485_gateway\kincony_a2_usb_rs485.yaml
esphome upload RS485_gateway\kincony_a2_usb_rs485.yaml --device COM5
```

`COM5` durch den tatsaechlichen Board-Port ersetzen. Danach in der
PC-Modbus-Software Modbus RTU, diesen COM-Port und die Slave-Adresse des
angeschlossenen Geraets einstellen. Vor Flashen den COM-Port dort schliessen.

Serielle ESPHome-Logs sind deaktiviert, damit sie keine Modbus-Daten
verunreinigen. Kein `esphome logs` oder serielles Terminal parallel zur
Modbus-Software oeffnen. ROM-Bootmeldungen des ESP32 koennen nach einem
Reset trotzdem auf USB erscheinen; vor der ersten Abfrage kurz warten und
den PC-Empfangspuffer leeren. DTR/RTS in der PC-Software deaktivieren bzw.
nicht toggeln, da die Auto-Reset-Schaltung sonst Reset/Bootloader ausloesen
kann. WLAN, Ethernet, Home-Assistant-API und OTA sind bewusst nicht aktiv.