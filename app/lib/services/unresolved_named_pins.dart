import '../models/action_config.dart';
import '../models/hardware_config.dart';
import '../models/macro_step.dart';
import '../models/pin_ref.dart';
import '../models/profile.dart';

/// EPIC-029 / TASK-391. Computes the set of named pin references in
/// a profile set that have no mapping in the active hardware config's
/// pinNames. UI banners use this to warn the builder before upload;
/// the firmware itself emits the equivalent count in its end-of-load
/// summary (TASK-380).
///
/// Empty set when nothing is unresolved — which is the steady state
/// for profiles that use direct GPIO refs only, or for builds with a
/// fully-populated pinNames map.
Set<String> findUnresolvedNamedPins({
  required List<Profile> profiles,
  required HardwareConfig hardwareConfig,
}) {
  final mapped = hardwareConfig.pinNames.values.toSet();
  final unresolved = <String>{};
  for (final profile in profiles) {
    for (final action in profile.buttons.values) {
      _walk(action, mapped, unresolved);
    }
  }
  return unresolved;
}

void _walk(ActionConfig action, Set<String> mapped, Set<String> out) {
  final ref = action.pinRef;
  if (ref is PinRefNamed && !mapped.contains(ref.name)) {
    out.add(ref.name);
  }
  // Recurse into wrappers — DelayedAction, longPress, doublePress, macro steps.
  final nested = action.action;
  if (nested != null) _walk(nested, mapped, out);
  final lp = action.longPress;
  if (lp != null) _walk(lp, mapped, out);
  final dp = action.doublePress;
  if (dp != null) _walk(dp, mapped, out);
  final steps = action.steps;
  if (steps != null) {
    for (final MacroStep step in steps) {
      for (final stepAction in step.actions) {
        _walk(stepAction, mapped, out);
      }
    }
  }
}
