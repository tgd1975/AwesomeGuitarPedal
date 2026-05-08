import 'package:awesome_studio_pedal/screens/legal_screen.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('renders app license + button to system third-party screen',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: LegalScreen()));
    await tester.pumpAndSettle();

    expect(find.text('AwesomeStudioPedal'), findsOneWidget);
    expect(find.textContaining('MIT License'), findsOneWidget);
    expect(find.text('View third-party licenses'), findsOneWidget);
  });

  testWidgets('tapping third-party button pushes a LicensePage route',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: LegalScreen()));
    await tester.pumpAndSettle();

    await tester.tap(find.text('View third-party licenses'));
    await tester.pumpAndSettle();

    // showLicensePage routes to LicensePage. The LegalScreen's
    // own AppBar title disappears; LicensePage's chrome takes over.
    expect(find.text('Legal & Open Source'), findsNothing);
    expect(find.byType(LicensePage), findsOneWidget);
  });
}
