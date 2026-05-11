#!/usr/bin/env python3
"""pedal_config.py — CLI tool for scanning, uploading, and validating pedal configs over BLE."""

import argparse
import asyncio
import json
import pathlib
import struct
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
PROFILES_SCHEMA = REPO_ROOT / "data" / "profiles.schema.json"
CONFIG_SCHEMA   = REPO_ROOT / "data" / "config.schema.json"

# UUIDs from docs/developers/BLE_CONFIG_PROTOCOL.md
SERVICE_UUID          = "516515c0-4b50-447b-8ca3-cbfce3f4d9f8"
CHAR_WRITE_UUID       = "516515c1-4b50-447b-8ca3-cbfce3f4d9f8"
CHAR_WRITE_HW_UUID    = "516515c2-4b50-447b-8ca3-cbfce3f4d9f8"
CHAR_STATUS_UUID      = "516515c3-4b50-447b-8ca3-cbfce3f4d9f8"
# Read-only: returns the board identity string ("esp32" / "nrf52840").
CHAR_HW_IDENTITY_UUID = "516515c4-4b50-447b-8ca3-cbfce3f4d9f8"

MTU          = 512
PAYLOAD_SIZE = MTU - 2   # 510 bytes
END_SEQ      = 0xFFFF

# Wire ceiling for an uploaded config. Mirrors MAX_CONFIG_BYTES in
# lib/PedalLogic/include/ble_config_reassembler.h — keep in sync. Pre-flight
# here so the user gets a clear "file too large" message instead of a
# multi-second BLE upload that ends in ERROR:parse_failed (TASK-240).
MAX_CONFIG_BYTES = 16 * 1024  # 16384


# ---------------------------------------------------------------------------
# Chunking helper (reusable by runner.py in test suite)
# ---------------------------------------------------------------------------

def make_chunks(data: bytes) -> list[bytes]:
    """Split *data* into MTU-sized BLE packets with 2-byte big-endian sequence numbers."""
    chunks = []
    seq = 0
    offset = 0
    while offset < len(data):
        payload = data[offset:offset + PAYLOAD_SIZE]
        chunks.append(struct.pack(">H", seq) + payload)
        seq += 1
        offset += PAYLOAD_SIZE
    # End-of-transfer sentinel: seq=0xFFFF, no payload
    chunks.append(struct.pack(">H", END_SEQ))
    return chunks


# ---------------------------------------------------------------------------
# Hardware identity helpers
# ---------------------------------------------------------------------------

def read_config_hardware(file_path: pathlib.Path) -> tuple[str | None, str | None]:
    """Return (hardware_value, error_message).

    Reads the ``"hardware"`` field from *file_path*.  Returns ``(None, msg)``
    if the file cannot be read, parsed, or the field is absent.
    """
    try:
        data = json.loads(file_path.read_text())
    except OSError as exc:
        return None, f"cannot read {file_path}: {exc}"
    except json.JSONDecodeError as exc:
        return None, f"JSON parse error in {file_path}: {exc}"

    hw = data.get("hardware")
    if not hw:
        return None, (
            f"'hardware' field missing from {file_path.name}. "
            "Re-export it from the Configuration Builder to add this field."
        )
    return hw, None


def check_hardware_match(config_hw: str, device_hw: str) -> tuple[bool, str]:
    """Return (ok, message).

    *ok* is True when the values match (case-insensitive).  The returned
    *message* is an error string on mismatch, empty on success.
    """
    if config_hw.lower() == device_hw.lower():
        return True, ""
    return False, (
        f"hardware mismatch: config targets '{config_hw}' "
        f"but connected device is '{device_hw}'. Upload aborted."
    )


# ---------------------------------------------------------------------------
# Schema validation (shared by `validate` and `upload` pre-flight)
# ---------------------------------------------------------------------------

def _cross_field_errors_config(data: dict) -> list[str]:
    """Cross-field invariants on config.json that JSON Schema (Draft 7) cannot express.

    These are also enforced by the pre-commit hook; both must agree.
    """
    errors: list[str] = []
    if "numButtons" in data and "buttonPins" in data \
            and data["numButtons"] != len(data["buttonPins"]):
        errors.append(
            f"numButtons ({data['numButtons']}) != len(buttonPins) ({len(data['buttonPins'])})"
        )
    if "numSelectLeds" in data and "ledSelect" in data \
            and data["numSelectLeds"] != len(data["ledSelect"]):
        errors.append(
            f"numSelectLeds ({data['numSelectLeds']}) != len(ledSelect) ({len(data['ledSelect'])})"
        )
    if "numProfiles" in data and "numSelectLeds" in data:
        max_profiles = (1 << data["numSelectLeds"]) - 1
        if data["numProfiles"] > max_profiles:
            errors.append(
                f"numProfiles ({data['numProfiles']}) exceeds 2^numSelectLeds-1 "
                f"({max_profiles}) for numSelectLeds={data['numSelectLeds']}"
            )
    return errors


def _validate_against_schema(json_path: pathlib.Path,
                             schema_path: pathlib.Path,
                             *, is_config: bool = False
                             ) -> tuple[int, list[str]]:
    """Validate *json_path* against *schema_path*.

    Returns ``(exit_code, error_lines)``. ``exit_code`` is 0 on success, 1 on
    any failure (file missing, parse error, schema mismatch, cross-field
    mismatch). ``error_lines`` are formatted for direct printing.
    """
    try:
        import jsonschema
    except ImportError:
        return 1, ["ERROR: jsonschema not installed. Run: pip install jsonschema"]

    if not json_path.exists():
        return 1, [f"ERROR: file not found: {json_path}"]
    if not schema_path.exists():
        return 1, [f"ERROR: schema not found: {schema_path}"]

    try:
        data   = json.loads(json_path.read_text())
        schema = json.loads(schema_path.read_text())
    except json.JSONDecodeError as exc:
        return 1, [f"ERROR: JSON parse error in {json_path}: {exc}"]

    error_lines: list[str] = []
    # EPIC-029 / TASK-378 — register sibling schemas so $refs like
    # "pin-names.schema.json" resolve from data/ without a network fetch.
    try:
        from referencing import Registry, Resource
        from referencing.jsonschema import DRAFT7
        registry = Registry()
        for sibling in schema_path.parent.glob("*.schema.json"):
            if sibling == schema_path:
                continue
            try:
                sibling_doc = json.loads(sibling.read_text())
            except json.JSONDecodeError:
                continue
            resource = Resource(contents=sibling_doc, specification=DRAFT7)
            registry = registry.with_resource(uri=sibling.name, resource=resource)
        validator = jsonschema.Draft7Validator(schema, registry=registry)
    except ImportError:
        # Older jsonschema (<4.18) used RefResolver instead of referencing.
        validator = jsonschema.Draft7Validator(schema)

    for err in validator.iter_errors(data):
        path = " > ".join(str(p) for p in err.absolute_path) or "(root)"
        error_lines.append(f"  {path}: {err.message}")

    if is_config:
        for msg in _cross_field_errors_config(data):
            error_lines.append(f"  (cross-field) {msg}")

    if error_lines:
        error_lines.append(f"FAIL: {len(error_lines)} schema error(s) in {json_path}")
        return 1, error_lines

    return 0, []


# ---------------------------------------------------------------------------
# Subcommand: validate
# ---------------------------------------------------------------------------

def cmd_validate(args: argparse.Namespace) -> int:
    json_path   = pathlib.Path(args.file)
    schema_path = CONFIG_SCHEMA if args.hw else PROFILES_SCHEMA

    code, errors = _validate_against_schema(
        json_path, schema_path, is_config=args.hw
    )
    if code != 0:
        for line in errors:
            print(line, file=sys.stderr if line.startswith("ERROR:") else sys.stdout)
        return code

    print(f"OK: {json_path} is valid")
    return 0


# ---------------------------------------------------------------------------
# Subcommand: scan
# ---------------------------------------------------------------------------

async def _scan() -> int:
    try:
        from bleak import BleakScanner
        from bleak.exc import BleakError
    except ImportError:
        print("ERROR: bleak not installed. Run: pip install bleak", file=sys.stderr)
        return 1

    print("Scanning for BLE devices (5 s)…")
    try:
        results = await BleakScanner.discover(timeout=5.0, return_adv=True)
    except BleakError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        print("Is Bluetooth enabled on this machine?", file=sys.stderr)
        return 1
    found = False
    for d, adv in results.values():
        uuids = [str(u).lower() for u in adv.service_uuids]
        marker = " ← pedal" if SERVICE_UUID.lower() in uuids else ""
        print(f"  {d.address}  RSSI={adv.rssi:4d}  {d.name or '(unknown)'}{marker}")
        if marker:
            found = True
    if not found:
        print("  (no pedal device advertising the Config service found)")
    return 0


def cmd_scan(args: argparse.Namespace) -> int:
    return asyncio.run(_scan())


# ---------------------------------------------------------------------------
# Subcommand: upload / upload-config
# ---------------------------------------------------------------------------

async def _upload(file_path: pathlib.Path, char_uuid: str,
                  config_hardware: str | None = None) -> int:
    """Upload *file_path* to the pedal via BLE.

    If *config_hardware* is given (upload-config path), reads the device's
    hardware identity characteristic before writing and aborts on mismatch.
    """
    try:
        from bleak import BleakScanner, BleakClient
        from bleak.exc import BleakError
    except ImportError:
        print("ERROR: bleak not installed. Run: pip install bleak", file=sys.stderr)
        return 1

    # Read and chunk payload
    try:
        payload = file_path.read_bytes()
    except OSError as exc:
        print(f"ERROR: cannot read {file_path}: {exc}", file=sys.stderr)
        return 1

    if len(payload) > MAX_CONFIG_BYTES:
        print(f"ERROR: {file_path} is {len(payload)} bytes — exceeds device "
              f"limit of {MAX_CONFIG_BYTES} bytes ({MAX_CONFIG_BYTES // 1024} KB). "
              f"Trim profiles or shorten action values and retry.",
              file=sys.stderr)
        return 1

    chunks = make_chunks(payload)
    print(f"Payload: {len(payload)} bytes → {len(chunks)} chunks (incl. sentinel)")

    # Discover pedal. Match by either (a) Config service UUID in advertisement
    # (test firmware path — the test build advertises the Config service UUID),
    # or (b) device name starting with the pedal's advertised prefix (production
    # firmware path — production keeps BleKeyboard's default advertisement with
    # HID UUID + keyboard appearance + name; the Config service UUID is in GATT
    # but not in the advertisement). The name may be truncated by BLE to
    # 15 chars, so we match a prefix.
    print("Scanning for pedal…")
    name_prefix = "AwesomeStudioPe"

    def _is_pedal(d, adv) -> bool:
        uuids = [str(u).lower() for u in (adv.service_uuids or [])]
        if SERVICE_UUID.lower() in uuids:
            return True
        name = (adv.local_name or d.name or "")
        return name.startswith(name_prefix)

    status_received: list[str] = []

    def on_notify(_char, data: bytearray) -> None:
        status_received.append(data.decode("utf-8", errors="replace"))

    # Wrap the entire BLE flow in a single BleakError catch so a disconnect
    # at any point (scan, connect, start_notify, mid-chunk write, stop_notify)
    # surfaces as a clean one-line error instead of a Python traceback.
    # bleak maps abrupt peer disconnects to BleakGATTProtocolError /
    # BleakDBusError / BleakError, all of which derive from BleakError.
    try:
        device = await BleakScanner.find_device_by_filter(_is_pedal, timeout=10.0)
        if device is None:
            print("ERROR: no pedal device found", file=sys.stderr)
            return 1

        print(f"Found: {device.name} ({device.address})")

        async with BleakClient(device) as client:

            # Hardware identity check (upload-config only)
            if config_hardware is not None:
                try:
                    raw = await client.read_gatt_char(CHAR_HW_IDENTITY_UUID)
                    device_hw = raw.decode("utf-8", errors="replace").strip()
                except BleakError:
                    raise  # let the outer handler surface the disconnect
                except Exception:
                    # Device predates TASK-180 firmware — warn but allow
                    print(
                        "WARNING: device did not expose hardware identity characteristic. "
                        "Upgrade firmware to enable mismatch protection. Proceeding anyway.",
                        file=sys.stderr,
                    )
                    device_hw = config_hardware  # treat as match

                ok, msg = check_hardware_match(config_hardware, device_hw)
                if not ok:
                    print(f"ERROR: {msg}", file=sys.stderr)
                    return 1
                print(f"Hardware identity verified: {device_hw}")

            await client.start_notify(CHAR_STATUS_UUID, on_notify)

            print(f"Uploading {len(chunks)} packets…")
            for i, chunk in enumerate(chunks):
                await client.write_gatt_char(char_uuid, chunk, response=True)
                if (i + 1) % 10 == 0:
                    print(f"  {i + 1}/{len(chunks)}")

            # Wait up to 10 s for status notification
            for _ in range(100):
                if status_received:
                    break
                await asyncio.sleep(0.1)

            await client.stop_notify(CHAR_STATUS_UUID)

    except BleakError as exc:
        print(
            f"ERROR: connection to pedal lost during upload ({exc}). "
            "Check that the pedal is powered and within range, then retry.",
            file=sys.stderr,
        )
        return 1

    if not status_received:
        print("ERROR: no CONFIG_STATUS notification received", file=sys.stderr)
        return 1

    status = status_received[-1]
    if status == "OK":
        print(f"OK: upload successful")
        return 0
    else:
        print(f"ERROR: pedal responded: {status}", file=sys.stderr)
        return 1


def cmd_upload(args: argparse.Namespace) -> int:
    is_config = bool(getattr(args, "hw", False))
    file_path = pathlib.Path(args.file)
    schema_path = CONFIG_SCHEMA if is_config else PROFILES_SCHEMA

    # Pre-flight schema validation — refuse to open BLE for a malformed file.
    # FEATURE_TEST_PLAN U-04 / U-06 / C-03 all relied on this guard.
    code, errors = _validate_against_schema(
        file_path, schema_path, is_config=is_config
    )
    if code != 0:
        for line in errors:
            print(line, file=sys.stderr if line.startswith("ERROR:") else sys.stdout)
        return code

    if is_config:
        # upload-config: extract hardware field for the device-identity check
        config_hw, err = read_config_hardware(file_path)
        if err:
            print(f"ERROR: {err}", file=sys.stderr)
            return 1
        return asyncio.run(_upload(file_path, CHAR_WRITE_HW_UUID, config_hardware=config_hw))
    else:
        return asyncio.run(_upload(file_path, CHAR_WRITE_UUID))


# ---------------------------------------------------------------------------
# Argument parser
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="pedal_config",
        description="AwesomeStudioPedal BLE config tool",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # scan
    sub.add_parser("scan", help="List nearby BLE devices advertising the pedal service")

    # upload
    up = sub.add_parser("upload", help="Upload profiles.json to pedal via BLE")
    up.add_argument("file", help="Path to profiles.json")

    # upload-config
    upc = sub.add_parser("upload-config", help="Upload config.json (hardware config) to pedal")
    upc.add_argument("file", help="Path to config.json")

    # validate
    val = sub.add_parser("validate", help="Validate JSON file against schema (no BLE needed)")
    val.add_argument("file", help="Path to JSON file")
    val.add_argument("--hw", action="store_true", help="Validate against config.schema.json instead")

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.command == "scan":
        return cmd_scan(args)
    elif args.command == "upload":
        args.hw = False
        return cmd_upload(args)
    elif args.command == "upload-config":
        args.hw = True
        return cmd_upload(args)
    elif args.command == "validate":
        return cmd_validate(args)
    else:
        parser.print_help()
        return 1


if __name__ == "__main__":
    sys.exit(main())
