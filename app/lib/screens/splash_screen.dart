import 'dart:async';

import 'package:flutter/material.dart';
import '../services/app_info.dart';
import '../theme/asp_theme.dart';

/// Min/max dwell are picked per IDEA-037: 600 ms feels intentional, 2 s is
/// the upper bound before the splash starts to feel like a hang.
class SplashTimings {
  const SplashTimings({
    this.minDwell = const Duration(milliseconds: 600),
    this.maxDwell = const Duration(seconds: 2),
  });

  final Duration minDwell;
  final Duration maxDwell;
}

class SplashScreen extends StatefulWidget {
  const SplashScreen({
    super.key,
    required this.onReady,
    this.timings = const SplashTimings(),
    this.initFuture,
  });

  static const pulseMarkKey = Key('splash_mark_pulse');
  static const staticMarkKey = Key('splash_mark_static');

  /// Called exactly once when the splash hands off (min-dwell elapsed AND
  /// init complete, or max-dwell elapsed regardless).
  final VoidCallback onReady;

  final SplashTimings timings;

  /// Optional async work to run during the splash. If null, the splash
  /// hands off after `minDwell`.
  final Future<void>? initFuture;

  @override
  State<SplashScreen> createState() => _SplashScreenState();
}

class _SplashScreenState extends State<SplashScreen> {
  bool _handedOff = false;
  Timer? _maxTimer;

  @override
  void initState() {
    super.initState();
    _maxTimer = Timer(widget.timings.maxDwell, _handOff);
    _runMinDwellWithInit();
  }

  Future<void> _runMinDwellWithInit() async {
    final init = widget.initFuture ?? Future<void>.value();
    await Future.wait<void>([
      Future<void>.delayed(widget.timings.minDwell),
      init,
    ]);
    _handOff();
  }

  void _handOff() {
    if (_handedOff) return;
    _handedOff = true;
    _maxTimer?.cancel();
    widget.onReady();
  }

  @override
  void dispose() {
    _maxTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final disableAnimations = MediaQuery.of(context).disableAnimations;
    return Scaffold(
      backgroundColor: AspTokens.bg,
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            _SplashMark(static: disableAnimations),
            const SizedBox(height: 16),
            Text(
              'AwesomeStudioPedal',
              style: Theme.of(context).textTheme.headlineMedium,
            ),
            const SizedBox(height: 32),
            FutureBuilder<AppInfo>(
              future: AppInfo.load(),
              builder: (context, snapshot) {
                final info = snapshot.data;
                if (info == null) return const SizedBox.shrink();
                return Text(
                  info.versionDisplay,
                  style: Theme.of(context).textTheme.labelMedium,
                );
              },
            ),
          ],
        ),
      ),
    );
  }
}

class _SplashMark extends StatefulWidget {
  const _SplashMark({required this.static});
  final bool static;

  @override
  State<_SplashMark> createState() => _SplashMarkState();
}

class _SplashMarkState extends State<_SplashMark>
    with SingleTickerProviderStateMixin {
  AnimationController? _controller;

  @override
  void initState() {
    super.initState();
    if (!widget.static) {
      _controller = AnimationController(
        vsync: this,
        duration: const Duration(milliseconds: 1200),
      )..repeat(reverse: true);
    }
  }

  @override
  void dispose() {
    _controller?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final image = Image.asset(
      'assets/branding/asp-mark-512.png',
      width: 96,
      height: 96,
    );
    final controller = _controller;
    if (widget.static || controller == null) {
      return KeyedSubtree(key: SplashScreen.staticMarkKey, child: image);
    }
    return AnimatedBuilder(
      key: SplashScreen.pulseMarkKey,
      animation: controller,
      builder: (_, child) => Opacity(
        opacity: 0.85 + 0.15 * controller.value,
        child: child,
      ),
      child: image,
    );
  }
}
