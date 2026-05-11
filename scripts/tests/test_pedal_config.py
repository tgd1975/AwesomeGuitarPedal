"""Unit tests for pedal_config.py (no BLE hardware required)."""

import asyncio
import io
import json
import pathlib
import struct
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from unittest.mock import MagicMock, patch

# Make scripts/ importable
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import pedal_config


# ---------------------------------------------------------------------------
# make_chunks
# ---------------------------------------------------------------------------

class TestMakeChunks(unittest.TestCase):

    def _seq(self, chunk: bytes) -> int:
        return struct.unpack(">H", chunk[:2])[0]

    def _payload(self, chunk: bytes) -> bytes:
        return chunk[2:]

    def test_empty_data_produces_only_sentinel(self):
        chunks = pedal_config.make_chunks(b"")
        self.assertEqual(len(chunks), 1)
        self.assertEqual(self._seq(chunks[0]), 0xFFFF)
        self.assertEqual(self._payload(chunks[0]), b"")

    def test_single_chunk_data(self):
        data = b"hello"
        chunks = pedal_config.make_chunks(data)
        self.assertEqual(len(chunks), 2)  # data chunk + sentinel
        self.assertEqual(self._seq(chunks[0]), 0)
        self.assertEqual(self._payload(chunks[0]), data)
        self.assertEqual(self._seq(chunks[1]), 0xFFFF)

    def test_exact_payload_size_is_one_chunk(self):
        data = b"x" * pedal_config.PAYLOAD_SIZE
        chunks = pedal_config.make_chunks(data)
        self.assertEqual(len(chunks), 2)
        self.assertEqual(self._seq(chunks[0]), 0)
        self.assertEqual(len(self._payload(chunks[0])), pedal_config.PAYLOAD_SIZE)

    def test_one_byte_over_splits_into_two_data_chunks(self):
        data = b"x" * (pedal_config.PAYLOAD_SIZE + 1)
        chunks = pedal_config.make_chunks(data)
        self.assertEqual(len(chunks), 3)  # 2 data + sentinel
        self.assertEqual(self._seq(chunks[0]), 0)
        self.assertEqual(self._seq(chunks[1]), 1)
        self.assertEqual(self._seq(chunks[2]), 0xFFFF)

    def test_sequence_numbers_are_contiguous(self):
        data = b"x" * (pedal_config.PAYLOAD_SIZE * 5 + 7)
        chunks = pedal_config.make_chunks(data)
        for i, chunk in enumerate(chunks[:-1]):  # skip sentinel
            self.assertEqual(self._seq(chunk), i)

    def test_reassembled_data_matches_input(self):
        data = b"abc" * 1000
        chunks = pedal_config.make_chunks(data)
        reassembled = b"".join(self._payload(c) for c in chunks[:-1])
        self.assertEqual(reassembled, data)

    def test_sentinel_always_last_and_empty(self):
        for size in [0, 1, pedal_config.PAYLOAD_SIZE, pedal_config.PAYLOAD_SIZE * 3]:
            with self.subTest(size=size):
                chunks = pedal_config.make_chunks(b"z" * size)
                self.assertEqual(self._seq(chunks[-1]), 0xFFFF)
                self.assertEqual(self._payload(chunks[-1]), b"")


# ---------------------------------------------------------------------------
# cmd_validate
# ---------------------------------------------------------------------------

VALID_PROFILES = {
    "profiles": [
        {
            "name": "Test",
            "buttons": {
                "A": {"type": "SendStringAction", "value": "hello"}
            }
        }
    ]
}

VALID_CONFIG = {
    "hardware": "esp32",
    "numProfiles": 3,           # ≤ 2^numSelectLeds - 1 = 3 (cross-field invariant)
    "numSelectLeds": 2,
    "numButtons": 2,            # == len(buttonPins) (cross-field invariant)
    "ledBluetooth": 26,
    "ledPower": 25,
    "ledSelect": [5, 18],       # == numSelectLeds (cross-field invariant)
    "buttonSelect": 21,
    "buttonPins": [13, 12]
}


class TestCmdValidate(unittest.TestCase):

    def _make_args(self, path: str, hw: bool = False) -> MagicMock:
        args = MagicMock()
        args.file = path
        args.hw = hw
        return args

    def test_valid_profiles_json_returns_0(self):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(VALID_PROFILES, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path))
        self.assertEqual(result, 0)

    def test_valid_config_json_returns_0(self):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(VALID_CONFIG, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path, hw=True))
        self.assertEqual(result, 0)

    def test_file_not_found_returns_1(self):
        result = pedal_config.cmd_validate(self._make_args("/nonexistent/file.json"))
        self.assertEqual(result, 1)

    def test_malformed_json_returns_1(self):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            f.write("{bad json{{")
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path))
        self.assertEqual(result, 1)

    def test_profiles_json_missing_required_field_returns_1(self):
        # profiles array is required
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump({"not_profiles": []}, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path))
        self.assertEqual(result, 1)

    def test_config_json_missing_required_field_returns_1(self):
        bad = dict(VALID_CONFIG)
        del bad["numProfiles"]
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(bad, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path, hw=True))
        self.assertEqual(result, 1)

    def test_profiles_json_empty_profiles_returns_1(self):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump({"profiles": []}, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path))
        self.assertEqual(result, 1)

    def test_profiles_json_more_than_seven_returns_1(self):
        # The 3-bit LED selector caps the addressable profile count at 7
        # (TASK-279). The schema enforces this with maxItems: 7.
        eight = {
            "profiles": [
                {"name": f"P{i + 1}", "buttons": {
                    "A": {"type": "SendKeyAction", "value": "KEY_A"}
                }}
                for i in range(8)
            ]
        }
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(eight, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path))
        self.assertEqual(result, 1)

    def test_shipped_profiles_json_is_valid(self):
        repo_root = pathlib.Path(__file__).resolve().parent.parent.parent
        shipped = repo_root / "data" / "profiles.json"
        result = pedal_config.cmd_validate(self._make_args(str(shipped)))
        self.assertEqual(result, 0)

    def test_missing_type_field_in_button_action_returns_1(self):
        bad = {
            "profiles": [{
                "name": "Bad",
                "buttons": {
                    "A": {"value": "hello"}  # missing "type"
                }
            }]
        }
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(bad, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path))
        self.assertEqual(result, 1)

    def test_config_pinnames_valid_mapping_returns_0(self):
        # EPIC-029 / TASK-378 — physical pin → standard role name.
        cfg = dict(VALID_CONFIG)
        cfg["pinNames"] = {"13": "button_a", "12": "button_b"}
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(cfg, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path, hw=True))
        self.assertEqual(result, 0)

    def test_config_pinnames_absent_is_valid(self):
        # Absence is the default; no behavioural regression for existing
        # configs (TASK-378 acceptance criterion).
        cfg = dict(VALID_CONFIG)
        cfg.pop("pinNames", None)
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(cfg, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path, hw=True))
        self.assertEqual(result, 0)

    def test_config_pinnames_unknown_name_returns_1(self):
        # Values must be a member of the v1 standard set
        # (data/pin-names.schema.json). Unknown names are rejected.
        cfg = dict(VALID_CONFIG)
        cfg["pinNames"] = {"13": "not_a_real_role_name"}
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(cfg, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path, hw=True))
        self.assertEqual(result, 1)

    def test_config_pinnames_partial_mapping_is_valid(self):
        # Builders may map some pins and not others — per-pin opt-in.
        cfg = dict(VALID_CONFIG)
        cfg["pinNames"] = {"13": "button_a"}  # buttonPins[1]=12 not mapped
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(cfg, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path, hw=True))
        self.assertEqual(result, 0)

    def test_config_pinnames_non_pin_key_returns_1(self):
        # Keys must be stringified GPIO numbers in 0..39. Non-numeric or
        # out-of-range keys are schema errors.
        cfg = dict(VALID_CONFIG)
        cfg["pinNames"] = {"D13": "button_a"}
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(cfg, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path, hw=True))
        self.assertEqual(result, 1)

    def test_invalid_action_type_string_returns_1(self):
        bad = {
            "profiles": [{
                "name": "Bad",
                "buttons": {
                    "A": {"type": "NotARealAction", "value": "hello"}
                }
            }]
        }
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(bad, f)
            path = f.name
        result = pedal_config.cmd_validate(self._make_args(path))
        self.assertEqual(result, 1)


# ---------------------------------------------------------------------------
# cmd_upload characteristic routing
# ---------------------------------------------------------------------------

class TestCmdUploadRouting(unittest.TestCase):
    """Verify upload vs upload-config targets the right GATT characteristic."""

    def test_upload_uses_profile_char(self):
        captured = {}

        async def fake_upload(file_path, char_uuid, config_hardware=None):
            captured["char_uuid"] = char_uuid
            captured["config_hardware"] = config_hardware
            return 0

        # Schema-valid input is required now that cmd_upload pre-flights the file
        # (TASK-233) — a missing/invalid file would short-circuit before _upload.
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(VALID_PROFILES, f)
            path = f.name

        with patch("pedal_config._upload", fake_upload):
            args = MagicMock()
            args.file = path
            args.hw = False
            pedal_config.cmd_upload(args)

        self.assertEqual(captured["char_uuid"], pedal_config.CHAR_WRITE_UUID)
        self.assertIsNone(captured["config_hardware"])

    def test_upload_config_uses_hw_char_and_passes_hardware(self):
        captured = {}

        async def fake_upload(file_path, char_uuid, config_hardware=None):
            captured["char_uuid"] = char_uuid
            captured["config_hardware"] = config_hardware
            return 0

        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(VALID_CONFIG, f)
            path = f.name

        with patch("pedal_config._upload", fake_upload):
            args = MagicMock()
            args.file = path
            args.hw = True
            pedal_config.cmd_upload(args)

        self.assertEqual(captured["char_uuid"], pedal_config.CHAR_WRITE_HW_UUID)
        self.assertEqual(captured["config_hardware"], "esp32")

    def test_upload_config_missing_hardware_field_returns_1(self):
        bad_config = dict(VALID_CONFIG)
        del bad_config["hardware"]

        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(bad_config, f)
            path = f.name

        args = MagicMock()
        args.file = path
        args.hw = True
        result = pedal_config.cmd_upload(args)
        self.assertEqual(result, 1)


# ---------------------------------------------------------------------------
# read_config_hardware
# ---------------------------------------------------------------------------

class TestReadConfigHardware(unittest.TestCase):

    def test_returns_hardware_value_from_valid_file(self):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(VALID_CONFIG, f)
            path = pathlib.Path(f.name)
        hw, err = pedal_config.read_config_hardware(path)
        self.assertEqual(hw, "esp32")
        self.assertIsNone(err)

    def test_returns_error_when_hardware_field_missing(self):
        cfg = dict(VALID_CONFIG)
        del cfg["hardware"]
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            json.dump(cfg, f)
            path = pathlib.Path(f.name)
        hw, err = pedal_config.read_config_hardware(path)
        self.assertIsNone(hw)
        self.assertIsNotNone(err)

    def test_returns_error_for_missing_file(self):
        hw, err = pedal_config.read_config_hardware(pathlib.Path("/nonexistent/config.json"))
        self.assertIsNone(hw)
        self.assertIsNotNone(err)

    def test_returns_error_for_malformed_json(self):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w", delete=False) as f:
            f.write("{bad json{{")
            path = pathlib.Path(f.name)
        hw, err = pedal_config.read_config_hardware(path)
        self.assertIsNone(hw)
        self.assertIsNotNone(err)


# ---------------------------------------------------------------------------
# check_hardware_match
# ---------------------------------------------------------------------------

class TestCheckHardwareMatch(unittest.TestCase):

    def test_matching_values_returns_true(self):
        ok, msg = pedal_config.check_hardware_match("esp32", "esp32")
        self.assertTrue(ok)
        self.assertEqual(msg, "")

    def test_matching_values_case_insensitive(self):
        ok, _ = pedal_config.check_hardware_match("ESP32", "esp32")
        self.assertTrue(ok)

    def test_mismatched_values_returns_false(self):
        ok, msg = pedal_config.check_hardware_match("nrf52840", "esp32")
        self.assertFalse(ok)
        self.assertIn("nrf52840", msg)
        self.assertIn("esp32", msg)

    def test_error_message_mentions_both_boards(self):
        ok, msg = pedal_config.check_hardware_match("esp32", "nrf52840")
        self.assertFalse(ok)
        self.assertIn("esp32", msg)
        self.assertIn("nrf52840", msg)


# ---------------------------------------------------------------------------
# build_parser / subcommand routing
# ---------------------------------------------------------------------------

class TestBuildParser(unittest.TestCase):

    def test_upload_subcommand_parsed(self):
        parser = pedal_config.build_parser()
        args = parser.parse_args(["upload", "profiles.json"])
        self.assertEqual(args.command, "upload")
        self.assertEqual(args.file, "profiles.json")

    def test_upload_config_subcommand_parsed(self):
        parser = pedal_config.build_parser()
        args = parser.parse_args(["upload-config", "config.json"])
        self.assertEqual(args.command, "upload-config")
        self.assertEqual(args.file, "config.json")

    def test_validate_subcommand_no_hw_flag(self):
        parser = pedal_config.build_parser()
        args = parser.parse_args(["validate", "profiles.json"])
        self.assertEqual(args.command, "validate")
        self.assertFalse(args.hw)

    def test_validate_subcommand_with_hw_flag(self):
        parser = pedal_config.build_parser()
        args = parser.parse_args(["validate", "config.json", "--hw"])
        self.assertTrue(args.hw)

    def test_scan_subcommand_parsed(self):
        parser = pedal_config.build_parser()
        args = parser.parse_args(["scan"])
        self.assertEqual(args.command, "scan")

    def test_missing_subcommand_raises(self):
        parser = pedal_config.build_parser()
        with self.assertRaises(SystemExit):
            parser.parse_args([])


# ---------------------------------------------------------------------------
# Constants sanity
# ---------------------------------------------------------------------------

class TestConstants(unittest.TestCase):

    def test_payload_size_is_mtu_minus_2(self):
        self.assertEqual(pedal_config.PAYLOAD_SIZE, pedal_config.MTU - 2)

    def test_end_seq_value(self):
        self.assertEqual(pedal_config.END_SEQ, 0xFFFF)

    def test_service_uuid_format(self):
        import re
        uuid_re = re.compile(
            r'^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$'
        )
        for uuid in [
            pedal_config.SERVICE_UUID,
            pedal_config.CHAR_WRITE_UUID,
            pedal_config.CHAR_WRITE_HW_UUID,
            pedal_config.CHAR_STATUS_UUID,
            pedal_config.CHAR_HW_IDENTITY_UUID,
        ]:
            with self.subTest(uuid=uuid):
                self.assertRegex(uuid, uuid_re)


# ---------------------------------------------------------------------------
# _scan — BLE discovery (mocked, no hardware)
# ---------------------------------------------------------------------------

class TestScan(unittest.TestCase):
    """Exercise _scan without a real BLE adapter by patching BleakScanner."""

    def _run_scan_with_discover(self, fake_discover):
        stdout = io.StringIO()
        stderr = io.StringIO()
        with patch("bleak.BleakScanner.discover", fake_discover):
            with redirect_stdout(stdout), redirect_stderr(stderr):
                exit_code = asyncio.run(pedal_config._scan())
        return exit_code, stdout.getvalue(), stderr.getvalue()

    def _device(self, address: str, name: str):
        dev = MagicMock()
        dev.address = address
        dev.name = name
        return dev

    def _adv(self, rssi: int, service_uuids):
        adv = MagicMock()
        adv.rssi = rssi
        adv.service_uuids = service_uuids
        return adv

    def test_happy_path_tags_pedal_device(self):
        pedal = self._device("AA:BB:CC:DD:EE:FF", "AwesomeStudioPe")
        other = self._device("11:22:33:44:55:66", "SomeOtherDevice")
        results = {
            pedal.address: (pedal, self._adv(-51, [pedal_config.SERVICE_UUID])),
            other.address: (other, self._adv(-80, ["0000180f-0000-1000-8000-00805f9b34fb"])),
        }

        async def fake_discover(timeout, return_adv=False):
            self.assertTrue(return_adv)
            return results

        exit_code, out, err = self._run_scan_with_discover(fake_discover)

        self.assertEqual(exit_code, 0)
        self.assertIn("AA:BB:CC:DD:EE:FF", out)
        self.assertIn("← pedal", out)
        self.assertIn("11:22:33:44:55:66", out)
        # "← pedal" must appear exactly once (only for the pedal device)
        self.assertEqual(out.count("← pedal"), 1)
        self.assertNotIn("(no pedal device", out)
        self.assertEqual(err, "")

    def test_no_pedal_prints_not_found_message(self):
        other = self._device("11:22:33:44:55:66", None)
        results = {
            other.address: (other, self._adv(-90, [])),
        }

        async def fake_discover(timeout, return_adv=False):
            return results

        exit_code, out, err = self._run_scan_with_discover(fake_discover)

        self.assertEqual(exit_code, 0)
        self.assertIn("(no pedal device advertising the Config service found)", out)
        self.assertNotIn("← pedal", out)
        self.assertEqual(err, "")

    def test_empty_results_prints_not_found_message(self):
        async def fake_discover(timeout, return_adv=False):
            return {}

        exit_code, out, err = self._run_scan_with_discover(fake_discover)

        self.assertEqual(exit_code, 0)
        self.assertIn("(no pedal device advertising the Config service found)", out)
        self.assertEqual(err, "")

    def test_bleak_error_handled_without_traceback(self):
        from bleak.exc import BleakError

        async def fake_discover(timeout, return_adv=False):
            raise BleakError("No powered Bluetooth adapters found.")

        exit_code, out, err = self._run_scan_with_discover(fake_discover)

        self.assertEqual(exit_code, 1)
        self.assertIn("ERROR:", err)
        self.assertIn("No powered Bluetooth adapters found", err)
        self.assertIn("Is Bluetooth enabled on this machine?", err)
        self.assertNotIn("Traceback", err)
        self.assertNotIn("Traceback", out)


# ---------------------------------------------------------------------------
# cmd_upload pre-flight schema validation (TASK-233)
# ---------------------------------------------------------------------------

class TestCmdUploadPreflightValidation(unittest.TestCase):
    """upload / upload-config must refuse a schema-invalid file before any BLE
    activity. Verified by replacing _upload with a sentinel that records whether
    it was called — the test asserts it is never invoked on the failing path.
    Covers FEATURE_TEST_PLAN.md §1.3 U-04, U-06, and §1.4 C-03.
    """

    def _run_upload_with_recording(self, file_contents, *, hw: bool):
        """Write *file_contents* to a temp file, call cmd_upload, return
        (exit_code, ble_called, stdout, stderr). ble_called is True iff
        the patched _upload was invoked."""
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w",
                                         delete=False) as f:
            if isinstance(file_contents, str):
                f.write(file_contents)
            else:
                json.dump(file_contents, f)
            path = f.name

        ble_called = {"flag": False}

        async def fake_upload(*args, **kwargs):
            ble_called["flag"] = True
            return 0

        args = MagicMock()
        args.file = path
        args.hw = hw

        stdout = io.StringIO()
        stderr = io.StringIO()
        with patch("pedal_config._upload", fake_upload):
            with redirect_stdout(stdout), redirect_stderr(stderr):
                exit_code = pedal_config.cmd_upload(args)

        return exit_code, ble_called["flag"], stdout.getvalue(), stderr.getvalue()

    def test_upload_profile_missing_buttons_field_does_not_open_ble(self):
        # FEATURE_TEST_PLAN U-04 reproduction
        bad = {"profiles": [{"name": "x"}]}  # missing required `buttons`
        code, ble, out, err = self._run_upload_with_recording(bad, hw=False)
        self.assertEqual(code, 1)
        self.assertFalse(ble, "BLE _upload must NOT be called for schema-invalid input")
        self.assertIn("buttons", out + err)

    def test_upload_empty_profiles_array_does_not_open_ble(self):
        # FEATURE_TEST_PLAN U-06 reproduction
        bad = {"profiles": []}
        code, ble, out, err = self._run_upload_with_recording(bad, hw=False)
        self.assertEqual(code, 1)
        self.assertFalse(ble)

    def test_upload_malformed_json_does_not_open_ble(self):
        code, ble, out, err = self._run_upload_with_recording(
            "{not even valid json", hw=False
        )
        self.assertEqual(code, 1)
        self.assertFalse(ble)

    def test_upload_config_cross_field_mismatch_does_not_open_ble(self):
        # FEATURE_TEST_PLAN C-03 reproduction:
        # numButtons=5 but buttonPins has only 4 entries — schema alone misses
        # this; the cross-field check catches it. Otherwise the missing pin
        # defaults to GPIO 0 (the ESP32 BOOT pin), which can interfere with boot.
        bad = dict(VALID_CONFIG)
        bad["numButtons"] = 5  # buttonPins still has 4 entries
        code, ble, out, err = self._run_upload_with_recording(bad, hw=True)
        self.assertEqual(code, 1)
        self.assertFalse(ble)
        self.assertIn("numButtons", out + err)

    def test_upload_config_missing_required_field_does_not_open_ble(self):
        bad = dict(VALID_CONFIG)
        del bad["numProfiles"]
        code, ble, out, err = self._run_upload_with_recording(bad, hw=True)
        self.assertEqual(code, 1)
        self.assertFalse(ble)

    def test_upload_valid_profile_proceeds_to_ble(self):
        code, ble, out, err = self._run_upload_with_recording(
            VALID_PROFILES, hw=False
        )
        self.assertEqual(code, 0)
        self.assertTrue(ble, "BLE _upload must be called for schema-valid input")

    def test_upload_config_valid_proceeds_to_ble(self):
        code, ble, out, err = self._run_upload_with_recording(
            VALID_CONFIG, hw=True
        )
        self.assertEqual(code, 0)
        self.assertTrue(ble)


# ---------------------------------------------------------------------------
# Cross-field config invariants in cmd_validate (TASK-233)
# ---------------------------------------------------------------------------

class TestCmdValidateCrossField(unittest.TestCase):
    """The cross-field validator must catch invariants JSON Schema cannot:
    numButtons == len(buttonPins), numSelectLeds == len(ledSelect),
    numProfiles ≤ 2^numSelectLeds - 1.
    """

    def _validate(self, data, *, hw=True):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="w",
                                         delete=False) as f:
            json.dump(data, f)
            path = f.name
        args = MagicMock()
        args.file = path
        args.hw = hw
        return pedal_config.cmd_validate(args)

    def test_buttonpins_length_mismatch_fails(self):
        bad = dict(VALID_CONFIG)
        bad["numButtons"] = 5
        # buttonPins still has 4 entries (the C-03 case)
        bad["buttonPins"] = [13, 12, 27, 14]
        self.assertEqual(self._validate(bad), 1)

    def test_ledselect_length_mismatch_fails(self):
        bad = dict(VALID_CONFIG)
        bad["numSelectLeds"] = 3
        bad["ledSelect"] = [5, 18]  # 2 entries, declared 3
        self.assertEqual(self._validate(bad), 1)

    def test_profiles_exceed_select_led_capacity_fails(self):
        bad = dict(VALID_CONFIG)
        bad["numProfiles"] = 8       # 2^numSelectLeds - 1 = 3
        self.assertEqual(self._validate(bad), 1)


# ---------------------------------------------------------------------------
# _upload — disconnect / BleakError handling (mocked, no hardware)
# ---------------------------------------------------------------------------

class TestUploadDisconnectErrorHandling(unittest.TestCase):
    """Verify _upload surfaces BLE disconnects as clean ERROR lines, not tracebacks.

    Covers FEATURE_TEST_PLAN.md §1.3 U-05: "Start upload, physically disconnect
    USB power from pedal mid-transfer → CLI reports connection loss clearly."
    Each test injects a BleakError at a different point in the upload flow.
    """

    def _fake_device(self):
        dev = MagicMock()
        dev.address = "AA:BB:CC:DD:EE:FF"
        dev.name = "AwesomeStudioPedal"
        return dev

    def _make_client_factory(self, *, fail_on: str | None,
                             status_payload: bytes = b"OK"):
        """Build a fake BleakClient class that raises BleakError at *fail_on*.

        *fail_on* may be one of: "aenter", "start_notify", "write_gatt_char",
        "stop_notify", "read_gatt_char", or None (no failure).
        """
        from bleak.exc import BleakError

        class FakeBleakClient:
            instances: list = []

            def __init__(self, device):
                self.device = device
                self._notify_cb = None
                FakeBleakClient.instances.append(self)

            async def __aenter__(self):
                if fail_on == "aenter":
                    raise BleakError("connect failed (peer unreachable)")
                return self

            async def __aexit__(self, *exc_info):
                return False

            async def read_gatt_char(self, _uuid):
                if fail_on == "read_gatt_char":
                    raise BleakError("read_gatt_char failed (disconnected)")
                return b"esp32"

            async def start_notify(self, _uuid, cb):
                if fail_on == "start_notify":
                    raise BleakError("start_notify failed (disconnected)")
                self._notify_cb = cb

            async def write_gatt_char(self, _uuid, _chunk, response=True):
                if fail_on == "write_gatt_char":
                    raise BleakError("GATT Protocol Error: Unlikely Error")
                # Deliver the status notification on the *last* successful write
                # so the loop can observe a status payload promptly.
                if self._notify_cb is not None:
                    self._notify_cb(None, bytearray(status_payload))

            async def stop_notify(self, _uuid):
                if fail_on == "stop_notify":
                    raise BleakError("stop_notify failed (disconnected)")

        return FakeBleakClient

    def _run_upload(self, *, fail_on, payload=b'{"profiles":[]}'):
        with tempfile.NamedTemporaryFile(suffix=".json", mode="wb",
                                         delete=False) as f:
            f.write(payload)
            path = pathlib.Path(f.name)

        FakeClient = self._make_client_factory(fail_on=fail_on)
        device = self._fake_device()

        async def fake_find(_filter, timeout=10.0):
            return device

        stdout = io.StringIO()
        stderr = io.StringIO()
        with patch("bleak.BleakScanner.find_device_by_filter", fake_find), \
             patch("bleak.BleakClient", FakeClient):
            with redirect_stdout(stdout), redirect_stderr(stderr):
                exit_code = asyncio.run(
                    pedal_config._upload(path, pedal_config.CHAR_WRITE_UUID)
                )

        return exit_code, stdout.getvalue(), stderr.getvalue()

    def _assert_clean_disconnect_error(self, exit_code, out, err):
        self.assertEqual(exit_code, 1)
        self.assertIn("ERROR: connection to pedal lost during upload", err)
        self.assertNotIn("Traceback", err)
        self.assertNotIn("Traceback", out)

    def test_aenter_failure_returns_1_without_traceback(self):
        exit_code, out, err = self._run_upload(fail_on="aenter")
        self._assert_clean_disconnect_error(exit_code, out, err)

    def test_start_notify_failure_returns_1_without_traceback(self):
        exit_code, out, err = self._run_upload(fail_on="start_notify")
        self._assert_clean_disconnect_error(exit_code, out, err)

    def test_write_gatt_char_failure_mid_chunk_returns_1_without_traceback(self):
        # 2 KB payload → multiple chunks, so write_gatt_char fires more than once
        exit_code, out, err = self._run_upload(
            fail_on="write_gatt_char", payload=b"x" * 2048
        )
        self._assert_clean_disconnect_error(exit_code, out, err)

    def test_stop_notify_failure_returns_1_without_traceback(self):
        exit_code, out, err = self._run_upload(fail_on="stop_notify")
        self._assert_clean_disconnect_error(exit_code, out, err)

    def test_happy_path_still_succeeds(self):
        exit_code, out, err = self._run_upload(fail_on=None)
        self.assertEqual(exit_code, 0, msg=f"stdout={out!r} stderr={err!r}")
        self.assertIn("OK: upload successful", out)
        self.assertNotIn("Traceback", err)


if __name__ == "__main__":
    unittest.main()
