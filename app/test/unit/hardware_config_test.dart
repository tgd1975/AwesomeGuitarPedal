import 'package:awesome_studio_pedal/models/hardware_config.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('HardwareConfig round-trip', () {
    final kBase = {
      'hardware': 'esp32',
      'numButtons': 2,
      'numProfiles': 4,
      'numSelectLeds': 2,
      'ledBluetooth': 26,
      'ledPower': 25,
      'ledSelect': [5, 18],
      'buttonSelect': 21,
      'buttonPins': [13, 12],
    };

    test('fromJson includes hardware field', () {
      final cfg = HardwareConfig.fromJson(kBase);
      expect(cfg.hardware, 'esp32');
    });

    test('toJson emits hardware field', () {
      final cfg = HardwareConfig.fromJson(kBase);
      expect(cfg.toJson()['hardware'], 'esp32');
    });

    test('full round-trip preserves all fields', () {
      final cfg = HardwareConfig.fromJson(kBase);
      final json = cfg.toJson();
      // pairing_pin is always emitted (null when absent in source).
      // debounceMs is always emitted with its default (100) when absent
      // in source (EPIC-028).
      expect(json, equals({...kBase, 'pairing_pin': null, 'debounceMs': 100}));
    });

    test('nrf52840 round-trip', () {
      final json = Map<String, dynamic>.from(kBase)..['hardware'] = 'nrf52840';
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.hardware, 'nrf52840');
      expect(cfg.toJson()['hardware'], 'nrf52840');
    });

    test('missing hardware field defaults to esp32', () {
      final json = Map<String, dynamic>.from(kBase)..remove('hardware');
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.hardware, 'esp32');
    });
  });

  group('pairing_pin', () {
    final kBase = {
      'hardware': 'esp32',
      'numButtons': 2,
      'numProfiles': 4,
      'numSelectLeds': 2,
      'ledBluetooth': 26,
      'ledPower': 25,
      'ledSelect': [5, 18],
      'buttonSelect': 21,
      'buttonPins': [13, 12],
    };

    test('absent pairing_pin → pairingPin is null', () {
      final cfg = HardwareConfig.fromJson(kBase);
      expect(cfg.pairingPin, isNull);
    });

    test('null pairing_pin → pairingPin is null', () {
      final json = Map<String, dynamic>.from(kBase)..['pairing_pin'] = null;
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.pairingPin, isNull);
    });

    test('integer pairing_pin is parsed', () {
      final json = Map<String, dynamic>.from(kBase)..['pairing_pin'] = 12345;
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.pairingPin, 12345);
    });

    test('pairing_pin round-trips through toJson', () {
      final json = Map<String, dynamic>.from(kBase)..['pairing_pin'] = 12345;
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.toJson()['pairing_pin'], 12345);
    });

    test('null pairing_pin round-trips as null in toJson', () {
      final cfg = HardwareConfig.fromJson(kBase);
      expect(cfg.toJson()['pairing_pin'], isNull);
    });

    test('zero pairing_pin is valid', () {
      final json = Map<String, dynamic>.from(kBase)..['pairing_pin'] = 0;
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.pairingPin, 0);
    });

    test('max valid pairing_pin (999999) is preserved', () {
      final json = Map<String, dynamic>.from(kBase)..['pairing_pin'] = 999999;
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.pairingPin, 999999);
    });
  });

  group('debounceMs', () {
    final kBase = {
      'hardware': 'esp32',
      'numButtons': 2,
      'numProfiles': 4,
      'numSelectLeds': 2,
      'ledBluetooth': 26,
      'ledPower': 25,
      'ledSelect': [5, 18],
      'buttonSelect': 21,
      'buttonPins': [13, 12],
    };

    test('absent debounceMs defaults to 100', () {
      final cfg = HardwareConfig.fromJson(kBase);
      expect(cfg.debounceMs, 100);
    });

    test('integer debounceMs is parsed', () {
      final json = Map<String, dynamic>.from(kBase)..['debounceMs'] = 50;
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.debounceMs, 50);
    });

    test('debounceMs round-trips through toJson', () {
      final json = Map<String, dynamic>.from(kBase)..['debounceMs'] = 250;
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.toJson()['debounceMs'], 250);
    });

    test('default debounceMs round-trips as 100 in toJson', () {
      final cfg = HardwareConfig.fromJson(kBase);
      expect(cfg.toJson()['debounceMs'], 100);
    });
  });

  group('boardTarget', () {
    final kBase = {
      'hardware': 'esp32',
      'numButtons': 2,
      'numProfiles': 4,
      'numSelectLeds': 2,
      'ledBluetooth': 26,
      'ledPower': 25,
      'ledSelect': [5, 18],
      'buttonSelect': 21,
      'buttonPins': [13, 12],
    };

    test('esp32 string maps to BoardTarget.esp32', () {
      final cfg = HardwareConfig.fromJson(kBase);
      expect(cfg.boardTarget, BoardTarget.esp32);
    });

    test('nrf52840 string maps to BoardTarget.nrf52840', () {
      final json = Map<String, dynamic>.from(kBase)..['hardware'] = 'nrf52840';
      final cfg = HardwareConfig.fromJson(json);
      expect(cfg.boardTarget, BoardTarget.nrf52840);
    });

    test('unknown string defaults to BoardTarget.esp32', () {
      expect(boardTargetFromString('unknown'), BoardTarget.esp32);
    });

    test('case-insensitive matching for ESP32', () {
      expect(boardTargetFromString('ESP32'), BoardTarget.esp32);
    });

    test('case-insensitive matching for NRF52840', () {
      expect(boardTargetFromString('NRF52840'), BoardTarget.nrf52840);
    });
  });
}
