import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:provider/provider.dart';
import '../models/action_config.dart';
import '../models/profiles_state.dart';

/// Editor for the top-level `independentActions` block in profiles.json.
/// Independent actions fire on every button event regardless of which profile
/// is active. Mirrors firmware semantics from TASK-306 and the surface choices
/// in the web simulator (TASK-307) and config builder (TASK-308).
class IndependentActionsScreen extends StatelessWidget {
  const IndependentActionsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<ProfilesState>();
    final slots = state.hardwareConfig?.buttonSlots ?? ['A', 'B', 'C', 'D'];

    return Scaffold(
      appBar: AppBar(
        title: const Text('Independent actions'),
        actions: [
          if (state.independentActions.isNotEmpty)
            IconButton(
              icon: const Icon(Icons.clear_all),
              tooltip: 'Remove all independent actions',
              onPressed: () => state.clearIndependentActions(),
            ),
        ],
      ),
      body: Column(
        children: [
          const Padding(
            padding: EdgeInsets.fromLTRB(16, 12, 16, 4),
            child: Text(
              'These actions fire on every button event regardless of which '
              'profile is active. They are not part of any profile and are '
              'not affected by profile switching.',
              style: TextStyle(fontSize: 13),
            ),
          ),
          Expanded(
            child: ListView.builder(
              itemCount: slots.length,
              itemBuilder: (context, i) {
                final slot = slots[i];
                final action = state.independentActions[slot];
                return ListTile(
                  leading: CircleAvatar(child: Text(slot)),
                  title: Text(action != null ? action.type : '(none)'),
                  subtitle: action?.name != null ? Text(action!.name!) : null,
                  trailing: action == null
                      ? const Icon(Icons.add)
                      : Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            IconButton(
                              icon: const Icon(Icons.delete_outline),
                              tooltip: 'Clear independent action $slot',
                              onPressed: () =>
                                  state.setIndependentAction(slot, null),
                            ),
                            const Icon(Icons.chevron_right),
                          ],
                        ),
                  onTap: () => context.push(
                    '/action/$slot',
                    extra: {
                      'action': action,
                      'onSave': (ActionConfig updated) {
                        state.setIndependentAction(slot, updated);
                      },
                    },
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
