import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/profiles_state.dart';
import '../services/unresolved_named_pins.dart';

/// EPIC-029 / TASK-391. Non-blocking banner that surfaces named pin
/// references in the loaded profiles that the active hardware config
/// has no mapping for. Shown in the profile editor / list and (via
/// reuse) on the connected-pedal screen.
///
/// The banner is informational, not an error: a builder may load a
/// community profile that uses names they don't need to map. The
/// firmware drops those actions silently with a warning log
/// (TASK-380); this banner makes the same condition visible before
/// the builder uploads.
class UnresolvedPinsBanner extends StatefulWidget {
  const UnresolvedPinsBanner({super.key});

  @override
  State<UnresolvedPinsBanner> createState() => _UnresolvedPinsBannerState();
}

class _UnresolvedPinsBannerState extends State<UnresolvedPinsBanner> {
  bool _dismissed = false;

  @override
  Widget build(BuildContext context) {
    if (_dismissed) return const SizedBox.shrink();
    // Defensive: some test contexts mount this screen without a
    // ProfilesState provider. The banner is purely advisory — render
    // nothing rather than crashing.
    ProfilesState state;
    try {
      state = context.watch<ProfilesState>();
    } on ProviderNotFoundException {
      return const SizedBox.shrink();
    }
    final hw = state.hardwareConfig;
    if (hw == null || hw.pinNames.isEmpty && state.profiles.isEmpty) {
      return const SizedBox.shrink();
    }
    final unresolved = findUnresolvedNamedPins(
      profiles: state.profiles,
      hardwareConfig: hw,
    );
    if (unresolved.isEmpty) return const SizedBox.shrink();

    final names = unresolved.toList()..sort();
    final color = Theme.of(context).colorScheme.tertiary;

    return MaterialBanner(
      backgroundColor: color.withValues(alpha: 0.12),
      leading: Icon(Icons.info_outline, color: color),
      content: Text(
        'Heads up — the active hardware config has no pinNames mapping for: '
        '${names.join(", ")}.\n'
        'These actions will be skipped when the pedal boots. '
        'This is a warning, not an error: you can upload anyway, or '
        'add the mapping in the hardware config.',
      ),
      actions: [
        TextButton(
          onPressed: () => setState(() => _dismissed = true),
          child: const Text('Dismiss'),
        ),
        TextButton(
          onPressed: () {
            // The hardware-config editor screen does not exist yet
            // (split into a future TASK). Surface the intent via a
            // snackbar so the build doesn't fail soundlessly.
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(
                content: Text(
                  'Hardware config editing is in data/config.json '
                  '(builder docs: docs/builders/HARDWARE_CONFIG.md). '
                  'In-app editor lands in a follow-up task.',
                ),
              ),
            );
          },
          child: const Text('Where do I map this?'),
        ),
      ],
    );
  }
}
