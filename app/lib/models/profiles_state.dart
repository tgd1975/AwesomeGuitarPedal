import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'action_config.dart';
import 'hardware_config.dart';
import 'profile.dart';

class ProfilesState extends ChangeNotifier {
  List<Profile> _profiles = [];
  HardwareConfig? _hardwareConfig;
  DateTime? _lastModified;
  // Top-level independentActions block from profiles.json. Fires on every
  // button event regardless of which profile is active. Stored slot-keyed.
  // An empty map means the block is omitted from the serialised JSON.
  Map<String, ActionConfig> _independentActions = {};

  List<Profile> get profiles => List.unmodifiable(_profiles);
  HardwareConfig? get hardwareConfig => _hardwareConfig;
  DateTime? get lastModified => _lastModified;
  Map<String, ActionConfig> get independentActions =>
      Map.unmodifiable(_independentActions);

  void loadFromJson(Map<String, dynamic> json) {
    _profiles = (json['profiles'] as List<dynamic>)
        .map((p) => Profile.fromJson(p as Map<String, dynamic>))
        .toList();
    final ind = json['independentActions'];
    if (ind is Map<String, dynamic>) {
      _independentActions = ind.map(
        (k, v) => MapEntry(k, ActionConfig.fromJson(v as Map<String, dynamic>)),
      );
    } else {
      _independentActions = {};
    }
    _lastModified = DateTime.now();
    notifyListeners();
  }

  void loadHardwareConfigFromJson(Map<String, dynamic> json) {
    _hardwareConfig = HardwareConfig.fromJson(json);
    notifyListeners();
  }

  void addProfile(Profile profile) {
    _profiles.add(profile);
    _lastModified = DateTime.now();
    notifyListeners();
  }

  void removeProfile(int index) {
    _profiles.removeAt(index);
    _lastModified = DateTime.now();
    notifyListeners();
  }

  void reorderProfiles(int oldIndex, int newIndex) {
    final item = _profiles.removeAt(oldIndex);
    _profiles.insert(newIndex, item);
    _lastModified = DateTime.now();
    notifyListeners();
  }

  void updateProfile(int index, Profile profile) {
    _profiles[index] = profile;
    _lastModified = DateTime.now();
    notifyListeners();
  }

  /// Replace the action mapped to [slot]. Pass null to clear that slot.
  void setIndependentAction(String slot, ActionConfig? action) {
    if (action == null) {
      _independentActions.remove(slot);
    } else {
      _independentActions[slot] = action;
    }
    _lastModified = DateTime.now();
    notifyListeners();
  }

  /// Drop every independent action so the block is omitted on next save.
  void clearIndependentActions() {
    if (_independentActions.isEmpty) return;
    _independentActions = {};
    _lastModified = DateTime.now();
    notifyListeners();
  }

  Map<String, dynamic> toProfilesJson() {
    final m = <String, dynamic>{
      'profiles': _profiles.map((p) => p.toJson()).toList(),
    };
    // Omit the key entirely when empty — never write {}. Mirrors firmware
    // contract from TASK-306 and the config builder.
    if (_independentActions.isNotEmpty) {
      m['independentActions'] = _independentActions.map(
        (k, v) => MapEntry(k, v.toJson()),
      );
    }
    return m;
  }

  String toProfilesJsonString() =>
      const JsonEncoder.withIndent('  ').convert(toProfilesJson());

  String toConfigJsonString() {
    if (_hardwareConfig == null) return '{}';
    return const JsonEncoder.withIndent('  ')
        .convert(_hardwareConfig!.toJson());
  }
}
