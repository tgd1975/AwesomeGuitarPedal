enum BoardTarget { esp32, nrf52840 }

BoardTarget boardTargetFromString(String s) {
  return switch (s.toLowerCase()) {
    'nrf52840' => BoardTarget.nrf52840,
    _ => BoardTarget.esp32,
  };
}

class HardwareConfig {
  HardwareConfig({
    required this.hardware,
    required this.numButtons,
    required this.numProfiles,
    required this.numSelectLeds,
    required this.ledBluetooth,
    required this.ledPower,
    required this.ledSelect,
    required this.buttonSelect,
    required this.buttonPins,
    this.pairingPin,
    this.debounceMs = 100,
    Map<int, String>? pinNames,
  }) : pinNames = Map.unmodifiable(pinNames ?? const <int, String>{});

  final String hardware;
  final int numButtons;
  final int numProfiles;
  final int numSelectLeds;
  final int ledBluetooth;
  final int ledPower;
  final List<int> ledSelect;
  final int buttonSelect;
  final List<int> buttonPins;

  /// BLE pairing passkey (0–999999). Null means no pairing required.
  final int? pairingPin;

  /// Project-wide button-debounce window in milliseconds (EPIC-028).
  /// Defaults to 100 ms when absent from the loaded config.
  final int debounceMs;

  /// Physical pin → standard role name (EPIC-029 / TASK-378). Empty when
  /// the loaded config omits `pinNames`. Names come from the v1 set in
  /// `data/pin-names.schema.json`; schema validation rejects unknown
  /// values before they reach this model.
  final Map<int, String> pinNames;

  BoardTarget get boardTarget => boardTargetFromString(hardware);

  factory HardwareConfig.fromJson(Map<String, dynamic> json) {
    final rawPin = json['pairing_pin'];
    final int? pairingPin =
        (rawPin != null && rawPin is num) ? rawPin.toInt() : null;

    final rawDebounce = json['debounceMs'];
    final int debounceMs = (rawDebounce is num) ? rawDebounce.toInt() : 100;

    final rawPinNames = json['pinNames'];
    final Map<int, String> pinNames = <int, String>{};
    if (rawPinNames is Map) {
      rawPinNames.forEach((key, value) {
        final pin = int.tryParse(key.toString());
        if (pin != null && value is String) {
          pinNames[pin] = value;
        }
      });
    }

    return HardwareConfig(
      hardware: json['hardware'] as String? ?? 'esp32',
      numButtons: (json['numButtons'] as num).toInt(),
      numProfiles: (json['numProfiles'] as num).toInt(),
      numSelectLeds: (json['numSelectLeds'] as num).toInt(),
      ledBluetooth: (json['ledBluetooth'] as num).toInt(),
      ledPower: (json['ledPower'] as num).toInt(),
      ledSelect: (json['ledSelect'] as List<dynamic>)
          .map((e) => (e as num).toInt())
          .toList(),
      buttonSelect: (json['buttonSelect'] as num).toInt(),
      buttonPins: (json['buttonPins'] as List<dynamic>)
          .map((e) => (e as num).toInt())
          .toList(),
      pairingPin: pairingPin,
      debounceMs: debounceMs,
      pinNames: pinNames,
    );
  }

  Map<String, dynamic> toJson() {
    final m = <String, dynamic>{
      'hardware': hardware,
      'numButtons': numButtons,
      'numProfiles': numProfiles,
      'numSelectLeds': numSelectLeds,
      'ledBluetooth': ledBluetooth,
      'ledPower': ledPower,
      'ledSelect': ledSelect,
      'buttonSelect': buttonSelect,
      'buttonPins': buttonPins,
      'pairing_pin':
          pairingPin, // null serialises as JSON null — disables pairing
      'debounceMs': debounceMs,
    };
    if (pinNames.isNotEmpty) {
      // Stringified-int keys for round-trip fidelity with the JSON schema.
      m['pinNames'] = {
        for (final entry in pinNames.entries) entry.key.toString(): entry.value
      };
    }
    return m;
  }

  /// Look up the standard role name mapped to *pin*, or null if the
  /// builder has not assigned a name to it. EPIC-029 / TASK-378.
  String? nameOf(int pin) => pinNames[pin];

  /// Look up the physical pin assigned to *name*, or null if no pin is
  /// mapped to that role on this hardware config. If the builder mapped
  /// the same name to multiple pins (warned about via [duplicatePinNames]
  /// but not rejected — TASK-378 decision), the lowest-numbered pin wins.
  /// EPIC-029 / TASK-378.
  int? pinOf(String name) {
    int? best;
    for (final entry in pinNames.entries) {
      if (entry.value == name && (best == null || entry.key < best)) {
        best = entry.key;
      }
    }
    return best;
  }

  /// Names that appear as the target of more than one pin in [pinNames].
  /// Empty when the mapping is one-to-one. UX layers surface this as a
  /// warning rather than a hard error (TASK-378 decision: JSON Schema
  /// cannot express value-uniqueness with patternProperties, and a
  /// schema error here would block legitimate local experimentation).
  Set<String> get duplicatePinNames {
    final seen = <String>{};
    final dupes = <String>{};
    for (final v in pinNames.values) {
      if (!seen.add(v)) dupes.add(v);
    }
    return dupes;
  }

  List<String> get buttonSlots {
    return List.generate(numButtons, (i) {
      return String.fromCharCode('A'.codeUnitAt(0) + i);
    });
  }
}
