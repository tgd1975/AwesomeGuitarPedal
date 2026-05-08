import 'package:awesome_studio_pedal/screens/troubleshooting_screen.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('renders the IDEA-037 example entries from default data',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: TroubleshootingScreen()));
    await tester.pumpAndSettle();

    expect(find.text('Pedal not found?'), findsOneWidget);
    expect(find.text('Buttons do nothing?'), findsOneWidget);
  });

  testWidgets('list is data-driven from injected entries', (tester) async {
    const entries = [
      TroubleshootingEntry(
        symptom: 'Test symptom',
        suggestion: 'Test suggestion',
      ),
    ];
    await tester.pumpWidget(
      const MaterialApp(home: TroubleshootingScreen(entries: entries)),
    );
    await tester.pumpAndSettle();

    expect(find.text('Test symptom'), findsOneWidget);
    expect(find.text('Test suggestion'), findsOneWidget);
    expect(find.text('Pedal not found?'), findsNothing);
  });
}
