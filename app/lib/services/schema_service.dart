import 'dart:convert';
import 'package:flutter/services.dart';
import 'package:json_schema/json_schema.dart';

import 'action_value_resolver.dart';

/// Result of profile-set validation. Two failure modes are distinguished:
///
/// - **Schema error** (`isSchemaError == true`): the JSON does not match
///   `profiles.schema.json` — e.g. a missing `buttons` field, an unknown
///   action `type`. The data is structurally invalid and importing it
///   would corrupt the in-memory state, so callers should reject the
///   import outright.
/// - **Runtime-resolvability error** (`isSchemaError == false` and
///   `errors` non-empty): the JSON is structurally valid but contains
///   action values the firmware will silently drop (e.g. `value:
///   "NOT_A_VALID_KEY_VALUE"`). The data can be loaded — the user should
///   be able to see and fix the bad values inside the app — but the
///   Profile List banner should turn red and the upload path should warn.
class ValidationResult {
  ValidationResult.valid()
      : isValid = true,
        isSchemaError = false,
        errors = const [];
  ValidationResult.invalid(this.errors, {this.isSchemaError = false})
      : isValid = false;

  final bool isValid;
  final bool isSchemaError;
  final List<String> errors;
}

class SchemaService {
  JsonSchema? _profilesSchema;
  JsonSchema? _configSchema;

  Future<void> _loadSchemas() async {
    if (_profilesSchema != null) return;
    final profilesRaw =
        await rootBundle.loadString('assets/profiles.schema.json');
    final configRaw = await rootBundle.loadString('assets/config.schema.json');
    final pinNamesRaw =
        await rootBundle.loadString('assets/pin-names.schema.json');

    // EPIC-029 / TASK-378: pre-register pin-names.schema.json as a sync
    // ref so JsonSchema.create() resolves "pin-names.schema.json" $refs
    // from the bundled asset instead of attempting a network fetch.
    final pinNames = jsonDecode(pinNamesRaw) as Map<String, dynamic>;
    final refProvider = RefProvider.sync((String ref) {
      if (ref == 'pin-names.schema.json' ||
          ref.endsWith('/pin-names.schema.json')) {
        return pinNames;
      }
      return null;
    });

    _profilesSchema =
        JsonSchema.create(jsonDecode(profilesRaw), refProvider: refProvider);
    _configSchema =
        JsonSchema.create(jsonDecode(configRaw), refProvider: refProvider);
  }

  Future<ValidationResult> validateProfiles(Map<String, dynamic> json) async {
    await _loadSchemas();
    final schemaResult = _profilesSchema!.validate(json);
    final schemaErrors = schemaResult.errors.map((e) => e.toString()).toList();

    // Schema-level errors are returned alone — the runtime resolver
    // assumes the JSON has the expected shape, so running it on a
    // malformed tree would just produce noise. Schema errors are
    // unrecoverable at the data layer; callers reject the import.
    if (schemaErrors.isNotEmpty) {
      return ValidationResult.invalid(schemaErrors, isSchemaError: true);
    }

    // Runtime resolvability: the schema permits any string for `value`,
    // but the firmware will silently drop actions whose value cannot be
    // resolved (named-key, hex/decimal in range, etc.). The data is
    // structurally valid, so callers may load it — but the Profile List
    // banner turns red so the user can fix the bad values before upload.
    // See TASK-264.
    final runtimeErrors = ActionValueResolver.findUnresolvable(json);
    if (runtimeErrors.isEmpty) return ValidationResult.valid();
    return ValidationResult.invalid(runtimeErrors, isSchemaError: false);
  }

  Future<ValidationResult> validateConfig(Map<String, dynamic> json) async {
    await _loadSchemas();
    final result = _configSchema!.validate(json);
    if (result.isValid) return ValidationResult.valid();
    return ValidationResult.invalid(
      result.errors.map((e) => e.toString()).toList(),
    );
  }
}
