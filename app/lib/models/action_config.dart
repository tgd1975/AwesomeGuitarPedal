import 'macro_step.dart';
import 'pin_ref.dart';

class ActionConfig {
  ActionConfig({
    required this.type,
    this.value,
    PinRef? pinRef,
    int? pin,
    this.delayMs,
    this.name,
    this.steps,
    this.action,
    this.longPress,
    this.doublePress,
  })  : assert(pinRef == null || pin == null,
            'Pass either pinRef or pin (back-compat), not both.'),
        pinRef = pinRef ?? (pin != null ? PinRefDirect(pin) : null);

  final String type;
  final String? value;

  /// Pin reference for Pin* actions. EPIC-029 / TASK-379. Either a
  /// direct GPIO number ([PinRefDirect]) or a standard role name
  /// ([PinRefNamed]). Schema validation rejects unknown role names
  /// upstream. Serialisation round-trips the authored form — `27`
  /// stays `27`, `button_a` stays `button_a` — so portability is not
  /// silently destroyed on save.
  final PinRef? pinRef;

  final int? delayMs;
  final String? name;
  // For MacroAction: list of steps, each step is a list of actions.
  final List<MacroStep>? steps;
  // For DelayedAction: the nested action.
  final ActionConfig? action;
  // Optional long-press / double-press sub-actions.
  final ActionConfig? longPress;
  final ActionConfig? doublePress;

  /// Back-compat accessor for call sites that only care about the
  /// direct-pin form (e.g. the legacy action editor UI before TASK-381
  /// rewires it for named pins). Returns null for [PinRefNamed]
  /// entries — those call sites should migrate to [pinRef].
  int? get pin => switch (pinRef) {
        final PinRefDirect d => d.pin,
        _ => null,
      };

  factory ActionConfig.fromJson(Map<String, dynamic> json) {
    return ActionConfig(
      type: json['type'] as String,
      value: json['value'] as String?,
      pinRef: PinRef.fromJson(json['pin']),
      delayMs: (json['delayMs'] as num?)?.toInt(),
      name: json['name'] as String?,
      action: json['action'] != null
          ? ActionConfig.fromJson(json['action'] as Map<String, dynamic>)
          : null,
      steps: json['steps'] != null
          ? (json['steps'] as List<dynamic>)
              .map((s) => MacroStep.fromJson(s as List<dynamic>))
              .toList()
          : null,
      longPress: json['longPress'] != null
          ? ActionConfig.fromJson(json['longPress'] as Map<String, dynamic>)
          : null,
      doublePress: json['doublePress'] != null
          ? ActionConfig.fromJson(json['doublePress'] as Map<String, dynamic>)
          : null,
    );
  }

  Map<String, dynamic> toJson() {
    final m = <String, dynamic>{'type': type};
    if (value != null) m['value'] = value;
    if (pinRef != null) m['pin'] = pinRef!.toJson();
    if (delayMs != null) m['delayMs'] = delayMs;
    if (name != null) m['name'] = name;
    if (action != null) m['action'] = action!.toJson();
    if (steps != null) m['steps'] = steps!.map((s) => s.toJson()).toList();
    if (longPress != null) m['longPress'] = longPress!.toJson();
    if (doublePress != null) m['doublePress'] = doublePress!.toJson();
    return m;
  }
}
