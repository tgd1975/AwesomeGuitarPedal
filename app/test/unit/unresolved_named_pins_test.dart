import 'package:awesome_studio_pedal/models/action_config.dart';
import 'package:awesome_studio_pedal/models/hardware_config.dart';
import 'package:awesome_studio_pedal/models/macro_step.dart';
import 'package:awesome_studio_pedal/models/pin_ref.dart';
import 'package:awesome_studio_pedal/models/profile.dart';
import 'package:awesome_studio_pedal/services/unresolved_named_pins.dart';
import 'package:flutter_test/flutter_test.dart';

HardwareConfig _hw({Map<int, String> pinNames = const {}}) => HardwareConfig(
      hardware: 'esp32',
      numButtons: 4,
      numProfiles: 3,
      numSelectLeds: 2,
      ledBluetooth: 26,
      ledPower: 25,
      ledSelect: const [5, 18],
      buttonSelect: 21,
      buttonPins: const [13, 12, 27, 14],
      pinNames: pinNames,
    );

Profile _profileWithAction(String slot, ActionConfig action) =>
    Profile(name: 'P', buttons: {slot: action});

void main() {
  group('findUnresolvedNamedPins (EPIC-029 / TASK-391)', () {
    test('returns empty set when no profiles have actions', () {
      final result = findUnresolvedNamedPins(
        profiles: [],
        hardwareConfig: _hw(),
      );
      expect(result, isEmpty);
    });

    test('direct-only profiles always return empty', () {
      final result = findUnresolvedNamedPins(
        profiles: [
          _profileWithAction(
            'A',
            ActionConfig(type: 'PinHighAction', pin: 27),
          ),
        ],
        hardwareConfig: _hw(),
      );
      expect(result, isEmpty);
    });

    test('named refs with full mapping return empty', () {
      final result = findUnresolvedNamedPins(
        profiles: [
          _profileWithAction(
            'A',
            ActionConfig(
              type: 'PinHighAction',
              pinRef: const PinRefNamed('button_a'),
            ),
          ),
        ],
        hardwareConfig: _hw(pinNames: const {13: 'button_a'}),
      );
      expect(result, isEmpty);
    });

    test('returns unresolved named refs', () {
      final result = findUnresolvedNamedPins(
        profiles: [
          _profileWithAction(
            'A',
            ActionConfig(
              type: 'PinHighAction',
              pinRef: const PinRefNamed('button_a'),
            ),
          ),
          _profileWithAction(
            'B',
            ActionConfig(
              type: 'PinLowAction',
              pinRef: const PinRefNamed('led_power'),
            ),
          ),
        ],
        hardwareConfig: _hw(pinNames: const {13: 'button_a'}),
      );
      expect(result, {'led_power'});
    });

    test('mixed direct + named — only unresolved named names are flagged', () {
      final result = findUnresolvedNamedPins(
        profiles: [
          Profile(name: 'P', buttons: {
            'A': ActionConfig(type: 'PinHighAction', pin: 27),
            'B': ActionConfig(
              type: 'PinLowAction',
              pinRef: const PinRefNamed('button_z'),
            ),
          }),
        ],
        hardwareConfig: _hw(pinNames: const {13: 'button_a'}),
      );
      expect(result, {'button_z'});
    });

    test('walks into nested DelayedAction.action', () {
      final inner = ActionConfig(
        type: 'PinHighAction',
        pinRef: const PinRefNamed('orphan'),
      );
      final outer = ActionConfig(
        type: 'DelayedAction',
        delayMs: 100,
        action: inner,
      );
      final result = findUnresolvedNamedPins(
        profiles: [_profileWithAction('A', outer)],
        hardwareConfig: _hw(),
      );
      expect(result, {'orphan'});
    });

    test('walks into longPress and doublePress variants', () {
      final action = ActionConfig(
        type: 'SendStringAction',
        value: 'hi',
        longPress: ActionConfig(
          type: 'PinHighAction',
          pinRef: const PinRefNamed('lp_orphan'),
        ),
        doublePress: ActionConfig(
          type: 'PinLowAction',
          pinRef: const PinRefNamed('dp_orphan'),
        ),
      );
      final result = findUnresolvedNamedPins(
        profiles: [_profileWithAction('A', action)],
        hardwareConfig: _hw(),
      );
      expect(result, {'lp_orphan', 'dp_orphan'});
    });

    test('walks into macro steps', () {
      final macro = ActionConfig(
        type: 'MacroAction',
        steps: [
          MacroStep(actions: [
            ActionConfig(
              type: 'PinHighAction',
              pinRef: const PinRefNamed('macro_orphan'),
            ),
          ]),
        ],
      );
      final result = findUnresolvedNamedPins(
        profiles: [_profileWithAction('A', macro)],
        hardwareConfig: _hw(),
      );
      expect(result, {'macro_orphan'});
    });
  });
}
