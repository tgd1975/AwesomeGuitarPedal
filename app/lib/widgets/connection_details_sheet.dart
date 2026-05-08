import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:provider/provider.dart';
import '../services/ble_service.dart';
import '../theme/asp_theme.dart';

void showConnectionDetailsSheet(BuildContext context) {
  showModalBottomSheet<void>(
    context: context,
    backgroundColor: AspTokens.surface,
    builder: (_) => const ConnectionDetailsSheet(),
  );
}

class ConnectionDetailsSheet extends StatelessWidget {
  const ConnectionDetailsSheet({super.key});

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<BleService>();
    final textTheme = Theme.of(context).textTheme;
    final connected = ble.isConnected;
    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Connection details', style: textTheme.titleLarge),
            const SizedBox(height: 16),
            _DetailRow(label: 'Device', value: ble.deviceName ?? '—'),
            const SizedBox(height: 8),
            _DetailRow(label: 'Identifier', value: ble.deviceId ?? '—'),
            const SizedBox(height: 8),
            // Pre-TASK-337: firmware version is not exposed via BLE yet.
            // Render '—' so the row's structure is stable and TASK-337
            // can fill it in by reading from BleService.
            const _DetailRow(label: 'Firmware', value: '—'),
            const SizedBox(height: 8),
            const _DetailRow(label: 'Signal', value: '—'),
            const SizedBox(height: 16),
            OutlinedButton.icon(
              icon: const Icon(Icons.developer_board),
              label: const Text('View pedal details'),
              onPressed: () {
                Navigator.of(context).maybePop();
                context.push('/connected-pedal');
              },
            ),
            const SizedBox(height: 8),
            if (connected) ...[
              OutlinedButton.icon(
                icon: const Icon(Icons.bluetooth_disabled),
                label: const Text('Disconnect'),
                onPressed: () {
                  ble.disconnect();
                  Navigator.of(context).maybePop();
                },
              ),
            ] else ...[
              OutlinedButton.icon(
                icon: const Icon(Icons.bluetooth_searching),
                label: const Text('Scan and connect'),
                onPressed: () {
                  Navigator.of(context).maybePop();
                  context.push('/scan');
                },
              ),
            ],
          ],
        ),
      ),
    );
  }
}

class _DetailRow extends StatelessWidget {
  const _DetailRow({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(
          width: 96,
          child: Text(label, style: textTheme.labelMedium),
        ),
        const SizedBox(width: 12),
        Expanded(
          child: Text(value, style: textTheme.bodyMedium),
        ),
      ],
    );
  }
}
