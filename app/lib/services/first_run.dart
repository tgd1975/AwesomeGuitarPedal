import 'package:shared_preferences/shared_preferences.dart';

class FirstRunGate {
  static const _howToDismissedKey = 'howto_first_run_dismissed';

  Future<bool> shouldAutoShowHowTo() async {
    final prefs = await SharedPreferences.getInstance();
    return !(prefs.getBool(_howToDismissedKey) ?? false);
  }

  Future<void> markHowToDismissed() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_howToDismissedKey, true);
  }
}

FirstRunGate firstRunGate = FirstRunGate();
