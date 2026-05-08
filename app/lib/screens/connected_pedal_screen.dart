import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/ble_service.dart';
import '../theme/asp_theme.dart';
import '../widgets/content_page_scaffold.dart';

class ConnectedPedalScreen extends StatefulWidget {
  const ConnectedPedalScreen({super.key});

  @override
  State<ConnectedPedalScreen> createState() => _ConnectedPedalScreenState();
}

class _ConnectedPedalScreenState extends State<ConnectedPedalScreen> {
  Future<String?>? _boardFuture;
  BleService? _watched;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    final ble = context.read<BleService>();
    if (ble != _watched) {
      _watched = ble;
      _refresh();
    }
  }

  void _refresh() {
    final ble = _watched;
    final next = (ble == null || !ble.isConnected)
        ? Future<String?>.value(null)
        : ble.readDeviceHardware();
    setState(() {
      _boardFuture = next;
    });
  }

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<BleService>();
    return ContentPageScaffold(
      title: 'Connected pedal',
      child: ListView(
        children: [
          if (!ble.isConnected) ...[
            const _DisconnectedBanner(),
            const SizedBox(height: 16),
          ],
          FutureBuilder<String?>(
            future: _boardFuture,
            builder: (context, snapshot) => _Row(
              label: 'Board',
              value: ble.isConnected ? (snapshot.data ?? '—') : '—',
            ),
          ),
          const Divider(height: 24),
          const _PendingRow(label: 'Firmware'),
          const Divider(height: 24),
          const _PendingRow(label: 'Configuration'),
          const Divider(height: 24),
          const _PendingRow(label: 'Storage'),
        ],
      ),
    );
  }
}

class _DisconnectedBanner extends StatelessWidget {
  const _DisconnectedBanner();

  @override
  Widget build(BuildContext context) => Container(
        padding: const EdgeInsets.all(12),
        decoration: BoxDecoration(
          color: AspTokens.surface,
          border: Border.all(color: AspTokens.border),
          borderRadius: BorderRadius.circular(AspTokens.radiusCard),
        ),
        child: Row(
          children: [
            const Icon(Icons.bluetooth_disabled,
                color: AspTokens.textMuted, size: 20),
            const SizedBox(width: 10),
            Expanded(
              child: Text(
                'Not connected. Pair a pedal to see live details.',
                style: Theme.of(context).textTheme.bodyMedium,
              ),
            ),
          ],
        ),
      );
}

class _Row extends StatelessWidget {
  const _Row({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(
          width: 120,
          child: Text(label, style: textTheme.labelMedium),
        ),
        const SizedBox(width: 12),
        Expanded(child: Text(value, style: textTheme.bodyMedium)),
      ],
    );
  }
}

class _PendingRow extends StatelessWidget {
  const _PendingRow({required this.label});

  final String label;

  static const _tooltip = 'Available after firmware update';

  @override
  Widget build(BuildContext context) => Tooltip(
        message: _tooltip,
        child: _Row(label: label, value: '— ($_tooltip)'),
      );
}
