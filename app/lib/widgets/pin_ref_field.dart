import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../models/hardware_config.dart';
import '../models/pin_ref.dart';
import '../services/pin_names_catalog.dart';

/// Pin-reference editor for Pin*Actions. EPIC-029 / TASK-390.
///
/// Two modes, builder toggles between them per pin reference:
///
/// - **Direct** — numeric GPIO input (the pre-EPIC-029 behaviour).
/// - **Named** — autocomplete-backed text input that picks from the
///   v1 standard set loaded via [PinNamesCatalog]. When the active
///   hardware config (passed via [hardwareConfig]) maps the chosen
///   name, an inline hint renders the resolved physical pin. A name
///   not in the v1 set is flagged with an inline validation message
///   pointing at the GitHub-addition process — the save path still
///   accepts it; downstream schema validation is authoritative.
///
/// The widget does NOT silently resolve a named ref into a direct
/// one on save. The portable form is the authored form (TASK-379
/// contract).
class PinRefField extends StatefulWidget {
  const PinRefField({
    super.key,
    this.initial,
    this.board = BoardTarget.esp32,
    this.hardwareConfig,
    required this.onChanged,
  });

  final PinRef? initial;
  final BoardTarget board;
  final HardwareConfig? hardwareConfig;
  final ValueChanged<PinRef?> onChanged;

  @override
  State<PinRefField> createState() => _PinRefFieldState();
}

enum _Mode { direct, named }

class _PinRefFieldState extends State<PinRefField> {
  late _Mode _mode;
  late TextEditingController _directCtrl;
  late TextEditingController _namedCtrl;
  PinNamesCatalog? _catalog;

  @override
  void initState() {
    super.initState();
    final initial = widget.initial;
    _mode = (initial is PinRefNamed) ? _Mode.named : _Mode.direct;
    _directCtrl = TextEditingController(
      text: (initial is PinRefDirect) ? initial.pin.toString() : '',
    );
    _namedCtrl = TextEditingController(
      text: (initial is PinRefNamed) ? initial.name : '',
    );
    PinNamesCatalog.load().then((c) {
      if (mounted) setState(() => _catalog = c);
    });
  }

  @override
  void dispose() {
    _directCtrl.dispose();
    _namedCtrl.dispose();
    super.dispose();
  }

  int _maxPinForBoard() => widget.board == BoardTarget.nrf52840 ? 47 : 39;

  void _emit() {
    if (_mode == _Mode.direct) {
      final pin = int.tryParse(_directCtrl.text);
      widget.onChanged(pin == null ? null : PinRefDirect(pin));
    } else {
      final name = _namedCtrl.text.trim();
      widget.onChanged(name.isEmpty ? null : PinRefNamed(name));
    }
  }

  void _switchMode(_Mode next) {
    setState(() => _mode = next);
    _emit();
  }

  /// Inline mapping hint shown below the named-pin input. Surfaces
  /// the active hardware config's mapping for the typed name, or a
  /// "no mapping" notice when the builder hasn't wired it on this
  /// build (per the TASK-380 firmware behaviour: load continues, the
  /// action drops).
  Widget _mappingHint() {
    final name = _namedCtrl.text.trim();
    if (name.isEmpty) return const SizedBox.shrink();
    final pin = widget.hardwareConfig?.pinOf(name);
    final inCatalog = _catalog?.contains(name) ?? true;
    if (!inCatalog) {
      return Padding(
        padding: const EdgeInsets.only(top: 4),
        child: Text(
          '"$name" is not in the v1 standard set. '
          'Propose an addition via the GitHub idea process (see TASK-382 docs).',
          style: TextStyle(color: Theme.of(context).colorScheme.error),
        ),
      );
    }
    if (pin == null) {
      return const Padding(
        padding: EdgeInsets.only(top: 4),
        child: Text(
          'No mapping in the active hardware config — action will be skipped on boot.',
        ),
      );
    }
    return Padding(
      padding: const EdgeInsets.only(top: 4),
      child: Text('→ GPIO $pin on this build'),
    );
  }

  Widget _directInput() {
    return TextField(
      controller: _directCtrl,
      keyboardType: TextInputType.number,
      inputFormatters: [FilteringTextInputFormatter.digitsOnly],
      decoration: InputDecoration(
        labelText: 'GPIO Pin (0–${_maxPinForBoard()})',
        helperText: 'Direct GPIO number — hardware-specific',
      ),
      onChanged: (_) => _emit(),
    );
  }

  Widget _namedInput() {
    final names = _catalog?.allNames ?? const <String>[];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Autocomplete<String>(
          initialValue: TextEditingValue(text: _namedCtrl.text),
          optionsBuilder: (textEditingValue) {
            if (_catalog == null) return const Iterable<String>.empty();
            return _catalog!.filterByPrefix(textEditingValue.text);
          },
          onSelected: (value) {
            _namedCtrl.text = value;
            _emit();
            setState(() {});
          },
          fieldViewBuilder: (context, controller, focusNode, onFieldSubmitted) {
            // Keep the local _namedCtrl in sync so the mapping hint
            // re-renders as the user types.
            controller.addListener(() {
              if (controller.text != _namedCtrl.text) {
                _namedCtrl.text = controller.text;
                _emit();
                setState(() {});
              }
            });
            return TextField(
              controller: controller,
              focusNode: focusNode,
              decoration: InputDecoration(
                labelText: 'Standard pin name',
                helperText:
                    'Portable — resolved against the hardware config\'s pinNames',
                hintText: names.isNotEmpty ? names.first : 'e.g. button_a',
              ),
              onSubmitted: (_) => onFieldSubmitted(),
            );
          },
        ),
        _mappingHint(),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        SegmentedButton<_Mode>(
          segments: const [
            ButtonSegment(value: _Mode.direct, label: Text('Direct')),
            ButtonSegment(value: _Mode.named, label: Text('Named')),
          ],
          selected: {_mode},
          onSelectionChanged: (s) => _switchMode(s.first),
        ),
        const SizedBox(height: 8),
        if (_mode == _Mode.direct) _directInput() else _namedInput(),
      ],
    );
  }
}
