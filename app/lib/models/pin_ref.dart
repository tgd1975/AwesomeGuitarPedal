// EPIC-029 / TASK-379. Sum type representing a pin reference inside a
// Pin* action. Two forms — pick one per occurrence; a single profile
// may mix both:
//
//  - PinRefDirect(int)    — direct GPIO number (0..39). Hardware-specific.
//  - PinRefNamed(String)  — standard role name from data/pin-names.schema.json.
//                           Portable; resolved against the active hardware
//                           config's pinNames map at use time (TASK-380 /
//                           TASK-381).
//
// Schema validation (data/profiles.schema.json#pinRef) constrains both
// arms before they reach this type, so an unknown role name never lands
// in PinRefNamed at runtime.

sealed class PinRef {
  const PinRef();

  /// Parses a JSON value from a `pin` field. Returns null when the
  /// value is absent or of an unexpected type (defensive — schema
  /// validation rejects bad types upstream).
  static PinRef? fromJson(dynamic raw) {
    if (raw is num) return PinRefDirect(raw.toInt());
    if (raw is String) return PinRefNamed(raw);
    return null;
  }

  /// Serialises back to the JSON form the builder authored.
  dynamic toJson();
}

class PinRefDirect extends PinRef {
  const PinRefDirect(this.pin);
  final int pin;

  @override
  int toJson() => pin;

  @override
  bool operator ==(Object other) => other is PinRefDirect && other.pin == pin;
  @override
  int get hashCode => Object.hash('direct', pin);

  @override
  String toString() => 'PinRefDirect($pin)';
}

class PinRefNamed extends PinRef {
  const PinRefNamed(this.name);
  final String name;

  @override
  String toJson() => name;

  @override
  bool operator ==(Object other) => other is PinRefNamed && other.name == name;
  @override
  int get hashCode => Object.hash('named', name);

  @override
  String toString() => 'PinRefNamed($name)';
}
