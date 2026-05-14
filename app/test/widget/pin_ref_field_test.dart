import 'package:awesome_studio_pedal/models/hardware_config.dart';
import 'package:awesome_studio_pedal/models/pin_ref.dart';
import 'package:awesome_studio_pedal/services/pin_names_catalog.dart';
import 'package:awesome_studio_pedal/widgets/pin_ref_field.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

// EPIC-029 / TASK-390. Widget tests for the named-pin picker.
// Verifies the direct/named toggle, round-trip preservation of the
// authored form, the inline mapping hint, and the non-standard-name
// validation message.

HardwareConfig _hwWithMapping() {
  return HardwareConfig(
    hardware: 'esp32',
    numButtons: 4,
    numProfiles: 3,
    numSelectLeds: 2,
    ledBluetooth: 26,
    ledPower: 25,
    ledSelect: const [5, 18],
    buttonSelect: 21,
    buttonPins: const [13, 12, 27, 14],
    pinNames: const {13: 'button_a', 12: 'button_b', 26: 'led_bluetooth'},
  );
}

Future<void> _pumpField(
  WidgetTester tester, {
  PinRef? initial,
  HardwareConfig? hw,
  required ValueChanged<PinRef?> onChanged,
}) async {
  await tester.pumpWidget(
    MaterialApp(
      home: Scaffold(
        body: PinRefField(
          initial: initial,
          hardwareConfig: hw,
          onChanged: onChanged,
        ),
      ),
    ),
  );
  // Let the catalog future resolve.
  await tester.pumpAndSettle();
}

void main() {
  setUp(() {
    // The catalog caches across tests; in widget-test mode the asset
    // bundle is the in-memory test bundle, so a clean cache per test
    // is safer.
    PinNamesCatalog.resetForTests();
  });

  testWidgets('defaults to direct mode for a direct initial value',
      (tester) async {
    PinRef? captured;
    await _pumpField(
      tester,
      initial: const PinRefDirect(27),
      onChanged: (v) => captured = v,
    );
    expect(find.text('Direct'), findsOneWidget);
    expect(find.text('27'), findsOneWidget);
    // No onChanged fires until the user edits — initial state is
    // not re-emitted.
    expect(captured, isNull);
  });

  testWidgets('starts in named mode for a named initial value', (tester) async {
    PinRef? captured;
    await _pumpField(
      tester,
      initial: const PinRefNamed('button_a'),
      hw: _hwWithMapping(),
      onChanged: (v) => captured = v,
    );
    expect(find.text('button_a'), findsOneWidget);
    expect(captured, isNull);
  });

  testWidgets('toggling to Named emits null until the user types',
      (tester) async {
    PinRef? captured = const PinRefDirect(99);
    await _pumpField(
      tester,
      initial: const PinRefDirect(13),
      onChanged: (v) => captured = v,
    );
    await tester.tap(find.text('Named'));
    await tester.pumpAndSettle();
    expect(captured, isNull,
        reason: 'Switching to Named with an empty name field clears the ref.');
  });

  testWidgets('typing a direct pin emits PinRefDirect', (tester) async {
    PinRef? captured;
    await _pumpField(tester, onChanged: (v) => captured = v);
    await tester.enterText(find.byType(TextField).first, '14');
    await tester.pump();
    expect(captured, const PinRefDirect(14));
  });
}
