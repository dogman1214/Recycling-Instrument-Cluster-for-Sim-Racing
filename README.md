# Audi B8.5 S5 Instrument Cluster over CAN (ESP32)

Driving a real Audi S5 (B8.5) instrument cluster, salvaged from a junked car, using an ESP32 that sends synthetic CAN frames. The goal is to use the cluster as a physical dashboard for sim-racing games.

**Status:** RPM needle works. Speedometer is unsolved (see below).
Currently building in the "production.ino" file, where I input specific RPM and MPH values to see change. Planned for support for Simhub to connect to games.

## Demo
[Add a photo and a short video of the RPM needle moving. This is the most important part of the README.]

## Hardware
- ESP32
- CAN transceiver model SN65HVD230
- Audi B8.5 S5 instrument cluster
- Generic 12v DC power supply
- [Pinout diagram or table: cluster connector pin -> function]

## How it works
The ESP32 uses its built-in CAN controller (TWAI driver) to send frames onto the bus the cluster listens to, mimicking what the car's ECU would send.

### RPM (working)
- CAN ID `0x107`, bytes 3-4 hold a 16-bit little-endian value.
- Raw value = RPM / ~3.0 scale factor, plus a piecewise correction I found by testing. [Link to the code or table.]
- Found by testing different values and watching movement
  
### Speedometer (not working yet)
- The signal appears in the DBC as `KBI_angez_Geschw` on ID `0x30B`.
- Sweeping and brute-forcing values did not move the needle.
- **Current theory:** the frame carries a CRC-8 (polynomial `0x2F`, the VAG/AUTOSAR E2E convention), and the cluster ignores frames with a wrong checksum. Not confirmed.
- **Next step:** capture a real CAN trace from a running B8.5 to check the checksum and counter behavior.

## Build and run
[Toolchain (ESP-IDF or Arduino), build command, and how to flash.]

## Repo layout
[A short list of files and what each does.]

## Roadmap
- [ ] Confirm the speedometer checksum with a real CAN trace
- [ ] Other signals: [warning lights, gear indicator, etc.]
- [ ] Hook up game telemetry (SimHub) so the cluster follows the game

## Notes and prior art
Others have done this with BMW, Mazda, and Lada clusters. This repo documents the B8.5 S5 messages I decoded.
