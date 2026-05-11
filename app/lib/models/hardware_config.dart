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
  });

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

  BoardTarget get boardTarget => boardTargetFromString(hardware);

  factory HardwareConfig.fromJson(Map<String, dynamic> json) {
    final rawPin = json['pairing_pin'];
    final int? pairingPin =
        (rawPin != null && rawPin is num) ? rawPin.toInt() : null;

    final rawDebounce = json['debounceMs'];
    final int debounceMs =
        (rawDebounce is num) ? rawDebounce.toInt() : 100;

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
    );
  }

  Map<String, dynamic> toJson() => {
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

  List<String> get buttonSlots {
    return List.generate(numButtons, (i) {
      return String.fromCharCode('A'.codeUnitAt(0) + i);
    });
  }
}
