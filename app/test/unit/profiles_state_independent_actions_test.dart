import 'package:awesome_studio_pedal/models/action_config.dart';
import 'package:awesome_studio_pedal/models/profiles_state.dart';
import 'package:flutter_test/flutter_test.dart';

/// TASK-309: ProfilesState round-trip tests for the optional top-level
/// `independentActions` block in profiles.json.
void main() {
  group('ProfilesState.independentActions', () {
    test('round-trip with block present preserves entries', () {
      final state = ProfilesState();
      state.loadFromJson({
        'profiles': [
          {
            'name': 'P1',
            'buttons': {
              'A': {'type': 'SendCharAction', 'value': 'KEY_PAGE_UP'},
            }
          }
        ],
        'independentActions': {
          'A': {'type': 'SerialOutputAction', 'value': 'tap', 'name': 'Log'},
          'C': {'type': 'PinHighWhilePressedAction', 'pin': 12},
        },
      });

      expect(state.independentActions.length, 2);
      expect(state.independentActions['A']!.type, 'SerialOutputAction');
      expect(state.independentActions['A']!.value, 'tap');
      expect(state.independentActions['A']!.name, 'Log');
      expect(state.independentActions['C']!.type, 'PinHighWhilePressedAction');
      expect(state.independentActions['C']!.pin, 12);

      final out = state.toProfilesJson();
      expect(out.containsKey('independentActions'), isTrue);
      final ind = out['independentActions'] as Map<String, dynamic>;
      final aJson = ind['A'] as Map<String, dynamic>;
      final cJson = ind['C'] as Map<String, dynamic>;
      expect(aJson['type'], 'SerialOutputAction');
      expect(aJson['value'], 'tap');
      expect(cJson['pin'], 12);
      // Slots that were never set are not added on serialise.
      expect(ind.containsKey('B'), isFalse);
      expect(ind.containsKey('D'), isFalse);
    });

    test('absent block leaves independentActions empty and omits the key', () {
      final state = ProfilesState();
      state.loadFromJson({
        'profiles': [
          {
            'name': 'P1',
            'buttons': {
              'A': {'type': 'SendCharAction', 'value': 'X'},
            }
          }
        ],
      });
      expect(state.independentActions, isEmpty);
      final out = state.toProfilesJson();
      expect(out.containsKey('independentActions'), isFalse,
          reason: 'must omit key entirely, not write {}');
    });

    test('setIndependentAction adds, replaces, and clears entries', () {
      final state = ProfilesState();
      state.loadFromJson({
        'profiles': [
          {'name': 'P1', 'buttons': <String, dynamic>{}}
        ],
      });

      // Add
      state.setIndependentAction(
          'A', ActionConfig(type: 'SerialOutputAction', value: 'hi'));
      expect(state.independentActions['A']!.value, 'hi');
      expect(state.toProfilesJson()['independentActions'], isNotNull);

      // Replace
      state.setIndependentAction(
          'A', ActionConfig(type: 'SerialOutputAction', value: 'bye'));
      expect(state.independentActions['A']!.value, 'bye');

      // Clear single slot
      state.setIndependentAction('A', null);
      expect(state.independentActions, isEmpty);
      expect(state.toProfilesJson().containsKey('independentActions'), isFalse,
          reason: 'last entry removed must drop the whole key');
    });

    test('clearIndependentActions empties the block and omits the JSON key',
        () {
      final state = ProfilesState();
      state.loadFromJson({
        'profiles': [
          {'name': 'P1', 'buttons': <String, dynamic>{}}
        ],
        'independentActions': {
          'A': {'type': 'SerialOutputAction', 'value': 'a'},
          'B': {'type': 'SerialOutputAction', 'value': 'b'},
        },
      });
      expect(state.independentActions.length, 2);

      state.clearIndependentActions();
      expect(state.independentActions, isEmpty);
      expect(state.toProfilesJson().containsKey('independentActions'), isFalse);
    });

    test('serialised entries are deeply equal to input (round-trip JSON)', () {
      final input = {
        'profiles': [
          {
            'name': 'P1',
            'buttons': <String, dynamic>{},
          }
        ],
        'independentActions': {
          'A': {
            'type': 'SerialOutputAction',
            'value': 'short',
            'longPress': {'type': 'SerialOutputAction', 'value': 'long'},
          },
        },
      };
      final state = ProfilesState();
      state.loadFromJson(input);
      final out = state.toProfilesJson();
      // Re-load from the output and confirm independents survive a second pass.
      final state2 = ProfilesState();
      state2.loadFromJson(out);
      expect(state2.independentActions['A']!.value, 'short');
      expect(state2.independentActions['A']!.longPress!.value, 'long');
    });

    test('change notifications fire on independent-action mutations', () {
      final state = ProfilesState();
      state.loadFromJson({
        'profiles': [
          {'name': 'P1', 'buttons': <String, dynamic>{}}
        ],
      });
      var notifications = 0;
      state.addListener(() => notifications++);

      state.setIndependentAction(
          'A', ActionConfig(type: 'SerialOutputAction', value: 'x'));
      state.setIndependentAction('A', null);
      state.clearIndependentActions(); // already empty — should not notify
      expect(notifications, 2,
          reason:
              'two real mutations notify; clearing an empty map is a no-op');
    });
  });
}
