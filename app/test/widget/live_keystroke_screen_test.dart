import 'package:awesome_studio_pedal/screens/live_keystroke_screen.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  setUp(() {
    HardwareKeyboard.instance.clearState();
  });

  testWidgets('printable character renders as a chip with that label',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: LiveKeystrokeScreen()));
    await tester.pumpAndSettle();

    await tester.sendKeyEvent(LogicalKeyboardKey.keyA);
    await tester.pumpAndSettle();

    expect(find.text('a'), findsOneWidget);
  });

  testWidgets('named-special key uses the table label and glyph',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: LiveKeystrokeScreen()));
    await tester.pumpAndSettle();

    await tester.sendKeyEvent(LogicalKeyboardKey.enter);
    await tester.pumpAndSettle();

    // Label is "Enter"; glyph "⏎" comes from overlay.yaml.
    expect(find.text('Enter'), findsOneWidget);
    expect(find.text('⏎'), findsOneWidget);
  });

  testWidgets('repeated identical key collapses with ×N', (tester) async {
    await tester.pumpWidget(const MaterialApp(home: LiveKeystrokeScreen()));
    await tester.pumpAndSettle();

    for (var i = 0; i < 3; i++) {
      await tester.sendKeyEvent(LogicalKeyboardKey.arrowDown);
    }
    await tester.pumpAndSettle();

    expect(find.text('Down'), findsOneWidget);
    expect(find.text('×3'), findsOneWidget);
  });

  testWidgets('modifier+key collapses into one chip with prefixed mod',
      (tester) async {
    await tester.pumpWidget(const MaterialApp(home: LiveKeystrokeScreen()));
    await tester.pumpAndSettle();

    await tester.sendKeyDownEvent(LogicalKeyboardKey.controlLeft);
    await tester.sendKeyDownEvent(LogicalKeyboardKey.shiftLeft);
    await tester.sendKeyEvent(LogicalKeyboardKey.keyS);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.shiftLeft);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.controlLeft);
    await tester.pumpAndSettle();

    // Modifiers render as separate _ModChip widgets prefixed before
    // the main chip; the row is a single visual unit.
    expect(find.text('Ctrl'), findsOneWidget);
    expect(find.text('Shift'), findsOneWidget);
    expect(find.text('s'), findsOneWidget);
  });

  testWidgets('Copy as text emits the documented plain form', (tester) async {
    final clipboardWrites = <String>[];
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(SystemChannels.platform, (call) async {
      if (call.method == 'Clipboard.setData') {
        final args = call.arguments as Map?;
        clipboardWrites.add(args?['text'] as String? ?? '');
      }
      return null;
    });
    addTearDown(() {
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
          .setMockMethodCallHandler(SystemChannels.platform, null);
    });

    await tester.pumpWidget(const MaterialApp(home: LiveKeystrokeScreen()));
    await tester.pumpAndSettle();

    // Build: Ctrl+Shift+S, Enter, Down ×3.
    await tester.sendKeyDownEvent(LogicalKeyboardKey.controlLeft);
    await tester.sendKeyDownEvent(LogicalKeyboardKey.shiftLeft);
    await tester.sendKeyEvent(LogicalKeyboardKey.keyS);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.shiftLeft);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.controlLeft);
    await tester.sendKeyEvent(LogicalKeyboardKey.enter);
    for (var i = 0; i < 3; i++) {
      await tester.sendKeyEvent(LogicalKeyboardKey.arrowDown);
    }
    await tester.pumpAndSettle();

    await tester.tap(find.text('Copy as text'));
    await tester.pumpAndSettle();

    expect(clipboardWrites, isNotEmpty);
    expect(clipboardWrites.last, 'Ctrl+Shift+s, Enter, Down ×3');
  });
}
