import 'dart:io';

import 'package:awesome_studio_pedal/services/pin_names_catalog.dart';
import 'package:flutter_test/flutter_test.dart';

// EPIC-029 / TASK-381a. The catalog parses pin-names.schema.json
// (the same asset shipped with the app). These tests load the file
// directly from disk instead of via rootBundle so they run without a
// Flutter binding.

void main() {
  late PinNamesCatalog catalog;

  setUpAll(() async {
    final path = '${Directory.current.path}/assets/pin-names.schema.json';
    final raw = await File(path).readAsString();
    catalog = PinNamesCatalog.fromJsonString(raw);
  });

  test('catalog exposes every name from x-categories', () {
    expect(catalog.allNames, isNotEmpty);
    expect(catalog.allNames, contains('button_a'));
    expect(catalog.allNames, contains('led_power'));
    expect(catalog.allNames, contains('button_select'));
  });

  test('catalog groups entries by category', () {
    final grouped = catalog.groupedByCategory();
    expect(
        grouped.keys,
        containsAll(<String>{
          'action_buttons',
          'profile_controls',
          'status_leds',
          'profile_select_leds',
        }));
    expect(grouped['action_buttons']!.map((e) => e.name),
        containsAll(<String>{'button_a', 'button_b'}));
  });

  test('contains returns true for v1 names and false for unknown ones', () {
    expect(catalog.contains('button_a'), isTrue);
    expect(catalog.contains('not_a_real_role'), isFalse);
  });

  test('lookup surfaces the role description for known names', () {
    final entry = catalog.lookup('led_power');
    expect(entry, isNotNull);
    expect(entry!.name, 'led_power');
    expect(entry.role, isNotEmpty);
    expect(entry.category, 'status_leds');
  });

  test('filterByPrefix is case-insensitive substring match', () {
    expect(catalog.filterByPrefix('led'),
        containsAll(<String>{'led_power', 'led_bluetooth'}));
    expect(catalog.filterByPrefix('BUTTON_A'), contains('button_a'));
    expect(catalog.filterByPrefix(''), equals(catalog.allNames));
    expect(catalog.filterByPrefix('zzz_no_match'), isEmpty);
  });
}
