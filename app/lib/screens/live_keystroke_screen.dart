import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../constants/hid_display_table.dart';
import '../theme/asp_theme.dart';
import '../widgets/content_page_scaffold.dart';

/// One row in the live-keystroke tape — a key, with optional modifier
/// names (e.g. "Ctrl", "Shift") that prefix the chip, plus a `count`
/// for ×N collapse of repeated identical events.
class KeystrokeChip {
  const KeystrokeChip({
    required this.label,
    required this.glyph,
    required this.icon,
    required this.modifiers,
    required this.usbHidUsage,
    required this.count,
  });

  final String label;
  final String? glyph;
  final IconData? icon;
  final List<String> modifiers;
  final int usbHidUsage;
  final int count;

  KeystrokeChip incremented() => KeystrokeChip(
        label: label,
        glyph: glyph,
        icon: icon,
        modifiers: modifiers,
        usbHidUsage: usbHidUsage,
        count: count + 1,
      );

  /// Plain-text form, e.g. "Ctrl+Shift+S" or "Vol+ ×3".
  String toPlainText() {
    final base = modifiers.isEmpty ? label : '${modifiers.join('+')}+$label';
    return count > 1 ? '$base ×$count' : base;
  }
}

class LiveKeystrokeScreen extends StatefulWidget {
  const LiveKeystrokeScreen({super.key});

  @override
  State<LiveKeystrokeScreen> createState() => _LiveKeystrokeScreenState();
}

class _LiveKeystrokeScreenState extends State<LiveKeystrokeScreen> {
  final FocusNode _focus = FocusNode();
  final List<KeystrokeChip> _chips = [];

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) => _focus.requestFocus());
  }

  @override
  void dispose() {
    _focus.dispose();
    super.dispose();
  }

  KeyEventResult _handle(KeyEvent event) {
    if (event is! KeyDownEvent) return KeyEventResult.ignored;
    final chip = _chipForEvent(event);
    if (chip == null) return KeyEventResult.ignored;
    setState(() {
      if (_chips.isNotEmpty &&
          _chips.first.usbHidUsage == chip.usbHidUsage &&
          _listEq(_chips.first.modifiers, chip.modifiers)) {
        _chips[0] = _chips.first.incremented();
      } else {
        _chips.insert(0, chip);
      }
    });
    return KeyEventResult.handled;
  }

  void _clear() => setState(_chips.clear);

  Future<void> _copyAsText() async {
    final text = _chips.reversed.map((c) => c.toPlainText()).join(', ');
    await Clipboard.setData(ClipboardData(text: text));
    if (!mounted) return;
    ScaffoldMessenger.of(context)
        .showSnackBar(const SnackBar(content: Text('Copied')));
  }

  @override
  Widget build(BuildContext context) => ContentPageScaffold(
        title: 'Live keystrokes',
        child: Focus(
          focusNode: _focus,
          autofocus: true,
          onKeyEvent: (_, e) => _handle(e),
          child: Column(
            children: [
              const _HelpBanner(),
              const SizedBox(height: 12),
              Row(
                children: [
                  OutlinedButton.icon(
                    icon: const Icon(Icons.delete_outline),
                    label: const Text('Clear'),
                    onPressed: _chips.isEmpty ? null : _clear,
                  ),
                  const SizedBox(width: 8),
                  OutlinedButton.icon(
                    icon: const Icon(Icons.copy),
                    label: const Text('Copy as text'),
                    onPressed: _chips.isEmpty ? null : _copyAsText,
                  ),
                ],
              ),
              const SizedBox(height: 12),
              Expanded(
                child: _chips.isEmpty
                    ? const _EmptyState()
                    : ListView.separated(
                        itemCount: _chips.length,
                        separatorBuilder: (_, __) => const SizedBox(height: 6),
                        itemBuilder: (_, i) => _ChipRow(chip: _chips[i]),
                      ),
              ),
            ],
          ),
        ),
      );
}

class _HelpBanner extends StatelessWidget {
  const _HelpBanner();

  @override
  Widget build(BuildContext context) => Container(
        padding: const EdgeInsets.all(12),
        decoration: BoxDecoration(
          color: AspTokens.surface,
          border: Border.all(color: AspTokens.border),
          borderRadius: BorderRadius.circular(AspTokens.radiusCard),
        ),
        child: Text(
          'Press a button on the connected pedal — the OS routes its '
          'HID keystrokes to this page. Works the same as any external '
          'keyboard.',
          style: Theme.of(context).textTheme.bodyMedium,
        ),
      );
}

class _EmptyState extends StatelessWidget {
  const _EmptyState();

  @override
  Widget build(BuildContext context) => Center(
        child: Text(
          'Waiting for keystrokes…',
          style: Theme.of(context).textTheme.labelMedium,
        ),
      );
}

class _ChipRow extends StatelessWidget {
  const _ChipRow({required this.chip});

  final KeystrokeChip chip;

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    final children = <Widget>[];
    for (final mod in chip.modifiers) {
      children.add(_ModChip(label: mod));
      children.add(const SizedBox(width: 4));
      children.add(Text('+', style: textTheme.bodyMedium));
      children.add(const SizedBox(width: 4));
    }
    children.add(_KeyChip(chip: chip));
    if (chip.count > 1) {
      children.add(const SizedBox(width: 8));
      children.add(Text('×${chip.count}', style: textTheme.labelMedium));
    }
    return Row(children: children);
  }
}

class _ModChip extends StatelessWidget {
  const _ModChip({required this.label});
  final String label;

  @override
  Widget build(BuildContext context) => Container(
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
        decoration: BoxDecoration(
          color: AspTokens.surface,
          borderRadius: BorderRadius.circular(AspTokens.radiusSm),
          border: Border.all(color: AspTokens.border),
        ),
        child: Text(label, style: Theme.of(context).textTheme.labelMedium),
      );
}

class _KeyChip extends StatelessWidget {
  const _KeyChip({required this.chip});
  final KeystrokeChip chip;

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    final inner = <Widget>[];
    if (chip.glyph != null) {
      inner.add(Text(chip.glyph!,
          style: textTheme.titleLarge?.copyWith(color: AspTokens.accent)));
      inner.add(const SizedBox(width: 6));
    } else if (chip.icon != null) {
      inner.add(Icon(chip.icon, color: AspTokens.accent, size: 18));
      inner.add(const SizedBox(width: 6));
    }
    inner.add(Text(chip.label, style: textTheme.bodyLarge));
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration: BoxDecoration(
        color: AspTokens.surfaceRaised,
        borderRadius: BorderRadius.circular(AspTokens.radiusSm),
        border: Border.all(color: AspTokens.borderStrong),
      ),
      child: Row(mainAxisSize: MainAxisSize.min, children: inner),
    );
  }
}

/// Maps a Flutter `KeyDownEvent` to a [KeystrokeChip] using
/// `kHidUsageDisplay` for named-special / glyph rendering, falling
/// back to printable character or `0xPPUUUU` hex.
KeystrokeChip? _chipForEvent(KeyEvent event) {
  final mods = _activeModifiers();
  // Drop standalone modifier presses — they appear in the next chip's
  // `modifiers` list when a real key follows.
  if (_isModifierUsage(event.physicalKey.usbHidUsage)) {
    return null;
  }
  final usage = event.physicalKey.usbHidUsage;
  final entry = kHidUsageDisplay[usage];
  if (entry != null) {
    return KeystrokeChip(
      label: entry.label,
      glyph: entry.glyph,
      icon: entry.icon,
      modifiers: mods,
      usbHidUsage: usage,
      count: 1,
    );
  }
  // Tier 1 — printable character.
  final printable = event.character;
  if (printable != null && printable.isNotEmpty && _isPrintable(printable)) {
    return KeystrokeChip(
      label: printable,
      glyph: null,
      icon: null,
      modifiers: mods,
      usbHidUsage: usage,
      count: 1,
    );
  }
  // Tier 3 — unmapped HID hex fallback.
  final page = (usage >> 16) & 0xFFFF;
  final id = usage & 0xFFFF;
  return KeystrokeChip(
    label: '0x${page.toRadixString(16).padLeft(2, '0').toUpperCase()}'
        ':0x${id.toRadixString(16).padLeft(4, '0').toUpperCase()}',
    glyph: null,
    icon: null,
    modifiers: mods,
    usbHidUsage: usage,
    count: 1,
  );
}

bool _isPrintable(String s) {
  final c = s.codeUnitAt(0);
  return c >= 0x20 && c != 0x7F;
}

bool _isModifierUsage(int usage) {
  // Keyboard/Keypad page modifier usage IDs are 0xE0–0xE7.
  // Packed form: (0x07 << 16) | usage_id → 0x000700E0 .. 0x000700E7.
  return usage >= 0x000700E0 && usage <= 0x000700E7;
}

List<String> _activeModifiers() {
  final pressed = HardwareKeyboard.instance.logicalKeysPressed;
  final out = <String>[];
  if (pressed.contains(LogicalKeyboardKey.controlLeft) ||
      pressed.contains(LogicalKeyboardKey.controlRight)) {
    out.add('Ctrl');
  }
  if (pressed.contains(LogicalKeyboardKey.shiftLeft) ||
      pressed.contains(LogicalKeyboardKey.shiftRight)) {
    out.add('Shift');
  }
  if (pressed.contains(LogicalKeyboardKey.altLeft) ||
      pressed.contains(LogicalKeyboardKey.altRight)) {
    out.add('Alt');
  }
  if (pressed.contains(LogicalKeyboardKey.metaLeft) ||
      pressed.contains(LogicalKeyboardKey.metaRight)) {
    out.add('GUI');
  }
  return out;
}

bool _listEq(List<String> a, List<String> b) {
  if (a.length != b.length) return false;
  for (var i = 0; i < a.length; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}
