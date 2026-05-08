import 'package:flutter/material.dart';
import '../services/first_run.dart';
import '../theme/asp_theme.dart';
import '../widgets/content_page_scaffold.dart';

class HowToScreen extends StatelessWidget {
  const HowToScreen({super.key, this.firstRun = false});

  final bool firstRun;

  @override
  Widget build(BuildContext context) {
    const body = _HowToContent();
    if (!firstRun) {
      return const ContentPageScaffold(
        title: 'How to use',
        child: SingleChildScrollView(child: body),
      );
    }
    return ContentPageScaffold(
      title: 'How to use',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          const Expanded(child: SingleChildScrollView(child: body)),
          const SizedBox(height: 12),
          ElevatedButton(
            onPressed: () async {
              await firstRunGate.markHowToDismissed();
              if (context.mounted) {
                await Navigator.of(context).maybePop();
              }
            },
            child: const Text("GOT IT, DON'T SHOW AGAIN"),
          ),
        ],
      ),
    );
  }
}

class _HowToContent extends StatelessWidget {
  const _HowToContent();

  static const _steps = <_Step>[
    _Step(
      icon: Icons.power_settings_new,
      title: 'Power on the pedal',
      detail: 'Plug it in or switch on the battery.',
    ),
    _Step(
      icon: Icons.bluetooth_searching,
      title: 'Pair over Bluetooth',
      detail: 'Tap "Connect to pedal" and pick your device.',
    ),
    _Step(
      icon: Icons.tune,
      title: 'Pick or edit a profile',
      detail: 'Use a community profile or build your own.',
    ),
    _Step(
      icon: Icons.upload,
      title: 'Send to the pedal',
      detail: 'Tap Upload — your profile is on the pedal.',
    ),
    _Step(
      icon: Icons.touch_app,
      title: 'Press a button',
      detail: 'The pedal sends the keystroke or macro.',
    ),
  ];

  @override
  Widget build(BuildContext context) => Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          for (var i = 0; i < _steps.length; i++) ...[
            if (i > 0) const SizedBox(height: 16),
            _StepRow(index: i + 1, step: _steps[i]),
          ],
        ],
      );
}

class _Step {
  const _Step({required this.icon, required this.title, required this.detail});
  final IconData icon;
  final String title;
  final String detail;
}

class _StepRow extends StatelessWidget {
  const _StepRow({required this.index, required this.step});

  final int index;
  final _Step step;

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Container(
          width: 32,
          height: 32,
          decoration: const BoxDecoration(
            color: AspTokens.surface,
            shape: BoxShape.circle,
          ),
          alignment: Alignment.center,
          child: Text('$index', style: textTheme.labelLarge),
        ),
        const SizedBox(width: 12),
        Icon(step.icon, color: AspTokens.accent, size: 24),
        const SizedBox(width: 12),
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(step.title, style: textTheme.titleLarge),
              const SizedBox(height: 2),
              Text(step.detail, style: textTheme.bodyMedium),
            ],
          ),
        ),
      ],
    );
  }
}
