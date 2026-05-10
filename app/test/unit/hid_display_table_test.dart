import 'package:awesome_studio_pedal/constants/hid_display_table.dart';
import 'package:flutter_test/flutter_test.dart';

int _packed(int page, int usageId) => (page << 16) | usageId;

void main() {
  group('kHidUsageDisplay', () {
    test('covers Keyboard/Keypad page (0x07) — Enter, letters, modifiers', () {
      expect(kHidUsageDisplay[_packed(0x07, 40)]?.label, 'Enter');
      expect(kHidUsageDisplay[_packed(0x07, 4)]?.label, 'a');
      expect(kHidUsageDisplay[_packed(0x07, 224)]?.label, 'Ctrl');
    });

    test('covers Consumer page (0x0C) — Volume, Play/Pause', () {
      expect(kHidUsageDisplay[_packed(0x0C, 233)]?.label, 'Volume Up');
      expect(kHidUsageDisplay[_packed(0x0C, 205)]?.label, 'Play/Pause');
    });

    test('overlay glyphs are merged onto generated entries', () {
      // Enter usage gets a glyph from overlay.yaml; the label still
      // comes from the generated entry.
      final enter = kHidUsageDisplay[_packed(0x07, 40)];
      expect(enter?.glyph, '⏎');
      expect(enter?.label, 'Enter');
    });

    test('overlay icons are merged onto Consumer-page entries', () {
      final volUp = kHidUsageDisplay[_packed(0x0C, 233)];
      expect(volUp?.icon, isNotNull);
    });

    test('every entry has a non-empty label and a category', () {
      for (final e in kHidUsageDisplay.values) {
        expect(e.label, isNotEmpty);
        expect(e.category, isNotNull);
      }
    });
  });
}
