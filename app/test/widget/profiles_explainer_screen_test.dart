import 'package:awesome_studio_pedal/screens/profiles_explainer_screen.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('renders explainer + two in-app navigation buttons',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: ProfilesExplainerScreen()));
    await tester.pumpAndSettle();

    expect(
      find.text('A profile is a mapping from buttons to actions.'),
      findsOneWidget,
    );
    expect(find.text('Open profile editor'), findsOneWidget);
    expect(find.text('Browse community profiles'), findsOneWidget);
  });
}
