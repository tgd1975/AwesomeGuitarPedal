import 'package:package_info_plus/package_info_plus.dart';

class AppInfo {
  AppInfo._({required this.version, required this.buildNumber});

  final String version;
  final String buildNumber;

  String get versionDisplay => 'v$version (build $buildNumber)';

  String get firmwareCompatibilityDisplay =>
      'Compatible with firmware of the same version.';

  static const githubRepoUrl = 'https://github.com/tgd1975/AwesomeStudioPedal';
  static const projectSiteUrl = 'https://tgd1975.github.io/AwesomeStudioPedal/';
  static const releaseNotesUrl =
      'https://github.com/tgd1975/AwesomeStudioPedal/releases';
  static const reportBugUrl =
      'https://github.com/tgd1975/AwesomeStudioPedal/issues/new';

  static const license = 'MIT';
  static const copyright = '© 2026 tgd1975';

  static AppInfo? _cached;

  static Future<AppInfo> load() async {
    final cached = _cached;
    if (cached != null) return cached;
    final info = await PackageInfo.fromPlatform();
    final loaded =
        AppInfo._(version: info.version, buildNumber: info.buildNumber);
    _cached = loaded;
    return loaded;
  }

  static void overrideForTesting(AppInfo info) {
    _cached = info;
  }

  static AppInfo forTesting({
    String version = '0.0.0',
    String buildNumber = '0',
  }) =>
      AppInfo._(version: version, buildNumber: buildNumber);

  static void resetForTesting() {
    _cached = null;
  }
}
