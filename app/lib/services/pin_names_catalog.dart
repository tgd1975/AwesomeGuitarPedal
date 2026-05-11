import 'dart:convert';
import 'package:flutter/services.dart';

/// One role-name entry in the v1 standard set, with category and a
/// human-readable role description for tooltips/help text.
class PinNameEntry {
  const PinNameEntry({
    required this.name,
    required this.category,
    required this.role,
  });
  final String name;
  final String category;
  final String role;
}

/// Loads and exposes the curated v1 pin-name vocabulary from
/// `data/pin-names.schema.json` (EPIC-029 / TASK-377). UI surfaces use
/// this for autocomplete, validation, and category-grouped pickers.
///
/// The schema's `enum` is the source of truth; the `x-categories`
/// metadata block adds grouping and role descriptions used purely for
/// presentation.
class PinNamesCatalog {
  PinNamesCatalog._(this._entries);

  final List<PinNameEntry> _entries;

  static PinNamesCatalog? _cached;

  /// Loads the catalog from the bundled asset on first call, then
  /// returns the cached instance. Safe to call repeatedly.
  static Future<PinNamesCatalog> load() async {
    if (_cached != null) return _cached!;
    final raw = await rootBundle.loadString('assets/pin-names.schema.json');
    return _cached = fromJsonString(raw);
  }

  /// Construct from a raw JSON string. Exposed for tests so they can
  /// inject a fixture without touching rootBundle.
  static PinNamesCatalog fromJsonString(String jsonString) {
    final doc = jsonDecode(jsonString) as Map<String, dynamic>;
    final categories =
        (doc['x-categories'] as Map<String, dynamic>? ?? const {});
    final entries = <PinNameEntry>[];
    categories.forEach((categoryKey, categoryValue) {
      final names =
          (categoryValue as Map<String, dynamic>)['names'] as List<dynamic>;
      for (final n in names) {
        final m = n as Map<String, dynamic>;
        entries.add(PinNameEntry(
          name: m['name'] as String,
          category: categoryKey,
          role: m['role'] as String? ?? '',
        ));
      }
    });
    return PinNamesCatalog._(entries);
  }

  /// Reset the static cache. Tests call this so each test sees a fresh
  /// load. Not for production use.
  static void resetForTests() {
    _cached = null;
  }

  /// Every name in the v1 set, insertion-ordered by category.
  List<String> get allNames => [for (final e in _entries) e.name];

  /// All entries, insertion-ordered. UI code can group on `category`.
  List<PinNameEntry> get entries => List.unmodifiable(_entries);

  /// Map of category-key → entries in that category. Insertion order
  /// preserved.
  Map<String, List<PinNameEntry>> groupedByCategory() {
    final out = <String, List<PinNameEntry>>{};
    for (final e in _entries) {
      out.putIfAbsent(e.category, () => []).add(e);
    }
    return out;
  }

  /// True iff *name* is a member of the v1 standard set.
  bool contains(String name) => _entries.any((e) => e.name == name);

  /// Lookup the catalog entry for *name*, or null if it is not in the
  /// v1 set.
  PinNameEntry? lookup(String name) {
    for (final e in _entries) {
      if (e.name == name) return e;
    }
    return null;
  }

  /// Substring-match against the v1 set for autocomplete dropdowns.
  /// Case-insensitive, returns names in insertion (category) order.
  List<String> filterByPrefix(String query) {
    if (query.isEmpty) return allNames;
    final q = query.toLowerCase();
    return [
      for (final e in _entries)
        if (e.name.toLowerCase().contains(q)) e.name,
    ];
  }
}
