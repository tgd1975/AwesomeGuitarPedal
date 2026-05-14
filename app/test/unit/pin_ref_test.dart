import 'package:awesome_studio_pedal/models/pin_ref.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('PinRef (EPIC-029 / TASK-379)', () {
    test('fromJson parses an int as PinRefDirect', () {
      expect(PinRef.fromJson(13), const PinRefDirect(13));
    });

    test('fromJson parses a string as PinRefNamed', () {
      expect(PinRef.fromJson('button_a'), const PinRefNamed('button_a'));
    });

    test('fromJson returns null for absent or unexpected types', () {
      expect(PinRef.fromJson(null), isNull);
      expect(PinRef.fromJson(<String, dynamic>{}), isNull);
      expect(PinRef.fromJson([1, 2, 3]), isNull);
    });

    test('toJson preserves the authored form', () {
      expect(const PinRefDirect(27).toJson(), 27);
      expect(const PinRefNamed('led_power').toJson(), 'led_power');
    });

    test('equality on PinRefDirect', () {
      expect(const PinRefDirect(5), const PinRefDirect(5));
      expect(const PinRefDirect(5), isNot(const PinRefDirect(6)));
    });

    test('equality on PinRefNamed', () {
      expect(const PinRefNamed('button_a'), const PinRefNamed('button_a'));
      expect(
          const PinRefNamed('button_a'), isNot(const PinRefNamed('button_b')));
    });

    test('PinRefDirect and PinRefNamed compare unequal even with same payload',
        () {
      expect(const PinRefDirect(5), isNot(const PinRefNamed('5')));
    });
  });
}
