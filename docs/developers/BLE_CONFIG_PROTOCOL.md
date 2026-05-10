# BLE Config Protocol

This document is the authoritative specification for the AwesomeStudioPedal BLE Config GATT
service. Firmware (TASK-118), Python CLI (TASK-119), and Flutter app (TASK-124) must all
implement this spec exactly.

## Service UUID

```
516515c0-4b50-447b-8ca3-cbfce3f4d9f8
```

This is a randomly generated 128-bit UUID unique to this project.

## Characteristics

| Characteristic     | Short name         | UUID                                   | Properties        |
|--------------------|--------------------|----------------------------------------|-------------------|
| Profile config     | `CONFIG_WRITE`     | `516515c1-4b50-447b-8ca3-cbfce3f4d9f8` | WRITE\_NO\_RESPONSE |
| Hardware config    | `CONFIG_WRITE_HW`  | `516515c2-4b50-447b-8ca3-cbfce3f4d9f8` | WRITE\_NO\_RESPONSE |
| Transfer status    | `CONFIG_STATUS`    | `516515c3-4b50-447b-8ca3-cbfce3f4d9f8` | NOTIFY            |

The UUID for each characteristic increments the last byte of the service UUID.

## Chunked Write Protocol

Both `CONFIG_WRITE` (profiles) and `CONFIG_WRITE_HW` (hardware config) use the same
chunked-transfer protocol:

### Packet format

Each BLE write packet is at most **512 bytes** (one ATT MTU):

```
[ sequence_number (2 bytes, big-endian) ][ payload (0–510 bytes) ]
```

- `sequence_number` starts at `0x0000` and increments by 1 per chunk.
- The payload is a raw slice of the UTF-8 JSON byte stream.
- The sender signals end-of-transfer with a final packet whose `sequence_number` is
  `0xFFFF` and whose payload is **empty** (2-byte packet total).

### Reassembly flow

```
Sender (CLI / app)                          Receiver (firmware)
───────────────────────────────────────────────────────────────
WRITE chunk seq=0x0000, payload=<bytes>  →  buffer[0..509]
WRITE chunk seq=0x0001, payload=<bytes>  →  buffer[510..1019]
…
WRITE chunk seq=0xFFFF, payload=<empty>  →  reassembly complete
                                         ←  NOTIFY CONFIG_STATUS "OK"  (on success)
                                         ←  NOTIFY CONFIG_STATUS "ERROR:<reason>"  (on failure)
```

### Error conditions

| Condition | `CONFIG_STATUS` notification | Action |
|-----------|------------------------------|--------|
| Out-of-order sequence | `ERROR:bad_sequence` | Transfer resets; buffer cleared |
| Payload exceeds `MAX_CONFIG_BYTES` (16 384 by default) | `ERROR:too_large` | Transfer resets |
| JSON parse failure | `ERROR:parse_failed` | Transfer resets |
| Config validation failure | `ERROR:invalid_config` | Transfer resets |
| A transfer already in progress | `BUSY` | New transfer rejected; ongoing transfer unaffected |

After any error, the receiver is ready to accept a fresh transfer immediately (sequence
resets to `0x0000`).

## `CONFIG_STATUS` Values

The `CONFIG_STATUS` characteristic notifies a UTF-8 string:

| Value | Meaning |
|-------|---------|
| `OK` | Transfer and apply succeeded |
| `BUSY` | A transfer is already in progress |
| `ERROR:<reason>` | Transfer failed; `<reason>` is one of the strings in the table above |

## Hardware Config Upload

Uploading a hardware config uses `CONFIG_WRITE_HW` with the same chunked protocol.
On success, the firmware:

1. Parses and validates `config.json` against the in-memory schema constraints.
2. Calls `loadHardwareConfig()` to apply the new pin assignments.
3. Saves the JSON to `/config.json` on LittleFS.
4. Notifies `CONFIG_STATUS "OK"`.

The new hardware config takes effect on the **next reboot** (live GPIO re-configuration
without a reset is out of scope).

## LED Confirmation

### Success (profiles or hardware config applied)

All profile-select LEDs blink **3 times** synchronously before the `OK` notification:

```
on 150 ms → off 150 ms → on 150 ms → off 150 ms → on 150 ms → off 150 ms
```

After blinking, `ProfileManager::updateLEDs()` is called to restore the current profile
indicator state. Total duration: 900 ms. The `OK` notification is sent immediately after
blinking completes.

### Failure

All profile-select LEDs blink **once** for 500 ms:

```
on 500 ms → off
```

## Concurrent Upload Rejection

If a client sends the first chunk of a new transfer while another transfer is in progress,
the firmware immediately notifies `BUSY` on `CONFIG_STATUS` and ignores the incoming packet.
The ongoing transfer continues uninterrupted.

## Coexistence with BLE HID

The Config GATT service is added to the same NimBLE stack that hosts the BLE HID profile.
Both services coexist in GATT. **The device operates as a normal BLE HID keyboard** — any
host that pairs with it (phone, tablet, Windows, macOS) performs GATT service discovery
after connecting and finds the full HID service.

The advertisement is whatever `BleKeyboard` emits by default — HID UUID
(`0x1812`), keyboard appearance (`0x03C1`), and the device name — so the pedal is
discoverable as a standard Bluetooth keyboard on Android, iOS, Windows, macOS, and Linux.
The Config service UUID is **not** advertised; clients that want to upload
configs (the Flutter app and `pedal_config.py` CLI) discover it via GATT service
discovery after connecting to the pedal by name or address.

The Config service does not interfere with keyboard report delivery — chunked writes are
processed in the main loop between HID events.

See [BLE_CONFIG_IMPLEMENTATION_NOTES.md](BLE_CONFIG_IMPLEMENTATION_NOTES.md) for a full
account of the BlueZ interference problem and the other engineering challenges encountered
during implementation.

## Constants

| Constant | Default value | Description |
|----------|--------------|-------------|
| `MAX_CONFIG_BYTES` | `16384` | Maximum reassembled JSON size (bytes) |
| `BLE_MTU` | `512` | ATT MTU; max chunk size = MTU − 2 = 510 payload bytes |

`MAX_CONFIG_BYTES` is defined in [`lib/PedalLogic/include/ble_config_reassembler.h`](../../lib/PedalLogic/include/ble_config_reassembler.h#L30) — that header is the authoritative source. If the values here disagree with the header, the header wins. Both can be overridden at compile time.
