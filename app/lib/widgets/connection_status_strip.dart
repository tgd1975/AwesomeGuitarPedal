import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/ble_service.dart';
import '../theme/asp_theme.dart';
import 'connection_details_sheet.dart';

class ConnectionStatusStrip extends StatelessWidget {
  const ConnectionStatusStrip({super.key});

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<BleService>();
    final connected = ble.isConnected;
    final name = ble.deviceName;
    final id = ble.deviceId;
    final shortId = id == null
        ? null
        : id.length > 6
            ? id.substring(id.length - 6)
            : id;

    final color = connected ? AspTokens.success : AspTokens.textMuted;
    final label = connected ? '${name ?? 'Pedal'} · $shortId' : 'Not connected';

    return InkWell(
      onTap: () => showConnectionDetailsSheet(context),
      child: Container(
        width: double.infinity,
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        decoration: const BoxDecoration(
          color: AspTokens.surface,
          border: Border(bottom: BorderSide(color: AspTokens.border)),
        ),
        child: Row(
          children: [
            Container(
              width: 8,
              height: 8,
              decoration: BoxDecoration(color: color, shape: BoxShape.circle),
            ),
            const SizedBox(width: 10),
            Expanded(
              child: Text(
                label,
                style: Theme.of(context).textTheme.labelMedium?.copyWith(
                      color: connected ? AspTokens.text : AspTokens.textMuted,
                    ),
              ),
            ),
            const Icon(Icons.chevron_right,
                size: 16, color: AspTokens.textMuted),
          ],
        ),
      ),
    );
  }
}
