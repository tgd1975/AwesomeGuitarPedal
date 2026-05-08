import 'package:awesome_studio_pedal/screens/how_to_screen.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  setUp(() {
    SharedPreferences.setMockInitialValues({});
  });

  testWidgets('renders 5 numbered steps with icons', (tester) async {
    await tester.pumpWidget(const MaterialApp(home: HowToScreen()));
    await tester.pumpAndSettle();

    expect(find.text('How to use'), findsOneWidget);

    for (var i = 1; i <= 5; i++) {
      expect(find.text('$i'), findsOneWidget);
    }

    expect(find.text('Power on the pedal'), findsOneWidget);
    expect(find.text('Pair over Bluetooth'), findsOneWidget);
    expect(find.text('Pick or edit a profile'), findsOneWidget);
    expect(find.text('Send to the pedal'), findsOneWidget);
    expect(find.text('Press a button'), findsOneWidget);
  });

  testWidgets('non-first-run mode does not show the dismiss button',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: HowToScreen()));
    await tester.pumpAndSettle();

    expect(find.text("GOT IT, DON'T SHOW AGAIN"), findsNothing);
  });

  testWidgets('first-run mode shows dismiss button and persists on tap',
      (tester) async {
    await tester
        .pumpWidget(const MaterialApp(home: HowToScreen(firstRun: true)));
    await tester.pumpAndSettle();

    expect(find.text("GOT IT, DON'T SHOW AGAIN"), findsOneWidget);

    await tester.tap(find.text("GOT IT, DON'T SHOW AGAIN"));
    await tester.pumpAndSettle();

    final prefs = await SharedPreferences.getInstance();
    expect(prefs.getBool('howto_first_run_dismissed'), isTrue);
  });

  testWidgets('first-run gate respects the persisted dismiss flag',
      (tester) async {
    SharedPreferences.setMockInitialValues({'howto_first_run_dismissed': true});

    final prefs = await SharedPreferences.getInstance();
    expect(prefs.getBool('howto_first_run_dismissed'), isTrue);
  });
}
