import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import '../constants/action_types.dart';
import '../models/action_config.dart';
import '../models/hardware_config.dart';
import '../models/pin_ref.dart';
import '../services/action_value_resolver.dart';
import '../widgets/action_type_dropdown.dart';
import '../widgets/key_value_field.dart';
import '../widgets/pin_ref_field.dart';

class ActionEditorScreen extends StatefulWidget {
  const ActionEditorScreen({
    super.key,
    required this.buttonId,
    this.initial,
    required this.onSave,
    this.embeddedMode = false,
    this.board = BoardTarget.esp32,
    this.hardwareConfig,
  });

  final String buttonId;
  final ActionConfig? initial;
  final void Function(ActionConfig) onSave;

  /// When true, renders as an inline widget rather than a full Scaffold screen.
  final bool embeddedMode;
  final BoardTarget board;

  /// Active hardware config — used by [PinRefField] to render the
  /// inline mapping hint next to named pin references. Optional so
  /// callers that don't have it (legacy code paths, tests) keep
  /// working; the hint just won't render.
  final HardwareConfig? hardwareConfig;

  @override
  State<ActionEditorScreen> createState() => _ActionEditorScreenState();
}

class _ActionEditorScreenState extends State<ActionEditorScreen> {
  late String _type;
  late TextEditingController _valueCtrl;
  late TextEditingController _nameCtrl;
  PinRef? _pinRef;

  @override
  void initState() {
    super.initState();
    _type = editorActionType(widget.initial?.type, widget.initial?.value);
    _valueCtrl = TextEditingController(text: widget.initial?.value ?? '');
    _nameCtrl = TextEditingController(text: widget.initial?.name ?? '');
    _pinRef = widget.initial?.pinRef;
  }

  @override
  void dispose() {
    _valueCtrl.dispose();
    _nameCtrl.dispose();
    super.dispose();
  }

  bool get _isPin => kPinTypes.contains(_type);
  bool get _needsValue =>
      _type == kKeyNamedSentinel ||
      _type == 'SendCharAction' ||
      _type == 'SendKeyAction' ||
      _type == 'SendStringAction' ||
      _type == 'SendMediaKeyAction' ||
      _type == 'SerialOutputAction';

  /// Categorises an action type by the value space its KeyValueField
  /// expects. Switching between types in different categories clears
  /// `_valueCtrl` so a stale value (e.g. `KEY_PAGE_UP` left over from
  /// "Key (named)" when the user picks "Media Key") cannot survive the
  /// dropdown change. See TASK-280 / idea-044.
  static String _valueSpaceOf(String type) {
    if (type == kKeyNamedSentinel || type == 'SendCharAction') return 'named';
    if (type == 'SendMediaKeyAction') return 'mediaKey';
    if (type == 'SendKeyAction') return 'rawHid';
    if (type == 'SendStringAction') return 'string';
    if (type == 'SerialOutputAction') return 'serial';
    return 'none';
  }

  void _onTypeChanged(String next) {
    setState(() {
      if (_valueSpaceOf(next) != _valueSpaceOf(_type)) {
        _valueCtrl.clear();
      }
      _type = next;
    });
  }

  /// Reason the current form state cannot be saved, or null if it can.
  /// Mirrors the same value-resolvability check the Profile List banner
  /// uses, so the user is blocked from saving a button that would silently
  /// fail at runtime. See TASK-264 / TASK-265.
  String? get _saveBlockedReason {
    if (_type.isEmpty) return 'Pick an action type to save.';
    if (_needsValue) {
      final resolved = ActionValueResolver.resolve(
        actionType: savedActionType(_type),
        value: _valueCtrl.text,
      );
      if (!resolved.isResolvable) return resolved.reason;
    }
    return null;
  }

  void _save() {
    final blocked = _saveBlockedReason;
    if (blocked != null) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Cannot save: $blocked')),
      );
      return;
    }
    final initial = widget.initial;
    final config = ActionConfig(
      type: savedActionType(_type),
      value: _needsValue ? _valueCtrl.text : null,
      pinRef: _isPin ? _pinRef : null,
      name: _nameCtrl.text.isEmpty ? null : _nameCtrl.text,
      longPress: initial?.longPress,
      doublePress: initial?.doublePress,
      steps: initial?.steps,
      action: initial?.action,
    );
    widget.onSave(config);
    if (context.canPop()) context.pop();
  }

  Widget _buildFields() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        ActionTypeDropdown(
          value: _type,
          onChanged: _onTypeChanged,
        ),
        const SizedBox(height: 16),
        if (_needsValue) KeyValueField(type: _type, controller: _valueCtrl),
        if (_isPin)
          PinRefField(
            initial: _pinRef,
            board: widget.board,
            hardwareConfig: widget.hardwareConfig,
            onChanged: (next) => setState(() => _pinRef = next),
          ),
        const SizedBox(height: 16),
        TextField(
          controller: _nameCtrl,
          decoration: const InputDecoration(
            labelText: 'Display name (optional)',
            hintText: 'e.g. Play/Pause',
          ),
        ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    if (widget.embeddedMode) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _buildFields(),
          const SizedBox(height: 8),
          Align(
            alignment: Alignment.centerRight,
            child: FilledButton(
              onPressed: _type.isNotEmpty ? _save : null,
              child: const Text('Apply'),
            ),
          ),
        ],
      );
    }
    return Scaffold(
      appBar: AppBar(
        title: Text('Button ${widget.buttonId}'),
        actions: [
          TextButton(
            onPressed: _type.isNotEmpty ? _save : null,
            child: const Text('Save'),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [_buildFields()],
      ),
    );
  }
}
