import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import '../widgets/content_page_scaffold.dart';

class ProfilesExplainerScreen extends StatelessWidget {
  const ProfilesExplainerScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    return ContentPageScaffold(
      title: 'Profiles',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'A profile is a mapping from buttons to actions.',
            style: textTheme.headlineMedium,
          ),
          const SizedBox(height: 16),
          Text(
            'Each profile says, for every button on the pedal, what to do '
            'on a single press, double press, or long press — send a '
            'keystroke, run a macro, switch profile.',
            style: textTheme.bodyLarge,
          ),
          const SizedBox(height: 12),
          Text(
            'You can save several profiles on the pedal and pick the '
            'active one with the SELECT button.',
            style: textTheme.bodyLarge,
          ),
          const SizedBox(height: 24),
          OutlinedButton.icon(
            icon: const Icon(Icons.tune),
            label: const Text('Open profile editor'),
            onPressed: () => context.push('/profiles'),
          ),
          const SizedBox(height: 8),
          OutlinedButton.icon(
            icon: const Icon(Icons.people),
            label: const Text('Browse community profiles'),
            onPressed: () => context.push('/community-profiles'),
          ),
        ],
      ),
    );
  }
}
