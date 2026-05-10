import 'package:flutter/material.dart';
import '../theme/asp_theme.dart';
import '../widgets/content_page_scaffold.dart';

class TroubleshootingEntry {
  const TroubleshootingEntry({required this.symptom, required this.suggestion});
  final String symptom;
  final String suggestion;
}

const troubleshootingEntries = <TroubleshootingEntry>[
  TroubleshootingEntry(
    symptom: 'Pedal not found?',
    suggestion: 'Check power. Make sure Bluetooth is on and you are in '
        'range. Move within 1–2 metres and tap "Connect to pedal" again.',
  ),
  TroubleshootingEntry(
    symptom: 'Buttons do nothing?',
    suggestion: 'Check the active profile on the pedal — press SELECT to '
        'switch profiles. Make sure you uploaded after editing.',
  ),
  TroubleshootingEntry(
    symptom: 'Upload fails or freezes?',
    suggestion: 'Disconnect, reconnect, and try again. If it keeps failing, '
        'restart the pedal (power cycle) and re-pair.',
  ),
  TroubleshootingEntry(
    symptom: 'Pairing keeps prompting?',
    suggestion: 'Remove the pedal in your phone\'s Bluetooth settings, '
        'then pair again from inside this app.',
  ),
];

class TroubleshootingScreen extends StatelessWidget {
  const TroubleshootingScreen(
      {super.key, this.entries = troubleshootingEntries});

  final List<TroubleshootingEntry> entries;

  @override
  Widget build(BuildContext context) => ContentPageScaffold(
        title: 'Troubleshooting',
        child: ListView.separated(
          itemCount: entries.length,
          separatorBuilder: (_, __) => const Divider(height: 24),
          itemBuilder: (_, i) => _TroubleshootingTile(entry: entries[i]),
        ),
      );
}

class _TroubleshootingTile extends StatelessWidget {
  const _TroubleshootingTile({required this.entry});

  final TroubleshootingEntry entry;

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Icon(Icons.help_outline, color: AspTokens.accent, size: 20),
        const SizedBox(width: 12),
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(entry.symptom, style: textTheme.titleLarge),
              const SizedBox(height: 4),
              Text(entry.suggestion, style: textTheme.bodyMedium),
            ],
          ),
        ),
      ],
    );
  }
}
