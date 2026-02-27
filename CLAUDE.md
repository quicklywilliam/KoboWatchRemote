# Kobo Page Turner - Apple Watch App

## Overview
WatchOS app that acts as a BLE central, connecting to a Kobo e-reader running as a BLE peripheral. The watch sends page turn commands (forward/back) via BLE writes.

## BLE Protocol

- **Kobo advertises as**: "Kobo Libra Colour"
- **Service UUID**: `0b278e49-7f56-4788-a1bb-4624e0d64b46`
- **Characteristic UUID**: `5257acb0-be4d-4cf1-af8f-cbdb67bf998a`
- **Characteristic properties**: Read, Write, Notify

### Commands (write to characteristic)
| Byte | Action |
|------|--------|
| `0x01` | Next page (forward) |
| `0x02` | Previous page (back) |

### Read response
Reading the characteristic returns a single byte with the last command received (or `0x00` if none).

## Architecture
- **WatchOS-only app** (no iOS companion needed for BLE central)
- Use `CoreBluetooth` framework (`CBCentralManager`, `CBPeripheral`)
- Scan for peripherals advertising service UUID `0b278e49-7f56-4788-a1bb-4624e0d64b46`
- Connect, discover services/characteristics, then write commands

## UI
- Simple two-button interface: "Next" and "Previous"
- Connection status indicator (scanning / connected / disconnected)
- Auto-reconnect on disconnect
- Digital Crown support: rotate forward = next page, rotate back = previous page

## Key Implementation Notes
- WatchOS BLE central mode is fully supported
- Use `CBCentralManager.scanForPeripherals(withServices:)` with the service UUID filter
- Write commands using `.withResponse` write type (the Kobo sends GATT responses)
- Handle `centralManagerDidUpdateState` to start scanning when BLE is ready
- Implement `didDisconnectPeripheral` to auto-reconnect

## Testing
- Can test BLE communication from Mac first using Python bleak:
  ```python
  import asyncio
  from bleak import BleakClient

  ADDR = "XX:XX:XX:XX:XX:XX"  # Kobo's BT address
  CHAR_UUID = "5257acb0-be4d-4cf1-af8f-cbdb67bf998a"

  async def main():
      async with BleakClient(ADDR) as client:
          # Next page
          await client.write_gatt_char(CHAR_UUID, bytes([0x01]))
          # Previous page
          await client.write_gatt_char(CHAR_UUID, bytes([0x02]))

  asyncio.run(main())
  ```

## Prerequisites
- Kobo must have BT enabled via its Settings UI
- `ble_peripheral` program must be running on the Kobo (`LD_LIBRARY_PATH=/usr/lib /tmp/ble_peripheral`)
- Kobo IP for SSH: typically `10.0.1.20`
