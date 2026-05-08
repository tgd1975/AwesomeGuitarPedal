import 'dart:async';

import 'package:awesome_studio_pedal/screens/splash_screen.dart';
import 'package:awesome_studio_pedal/services/app_info.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  setUp(() {
    AppInfo.overrideForTesting(
      AppInfo.forTesting(version: '0.5.0', buildNumber: '1'),
    );
  });

  tearDown(() => AppInfo.resetForTesting());

  testWidgets('renders mark, wordmark, and version', (tester) async {
    await tester.pumpWidget(MaterialApp(
      home: MediaQuery(
        // disableAnimations stops the infinite AnimationController so the
        // FutureBuilder for AppInfo can settle in pumpAndSettle.
        data: const MediaQueryData(disableAnimations: true),
        child: SplashScreen(
          onReady: () {},
          timings: const SplashTimings(
            minDwell: Duration(milliseconds: 50),
            maxDwell: Duration(milliseconds: 100),
          ),
          initFuture: Future<void>.value(),
        ),
      ),
    ));
    await tester.pumpAndSettle();

    expect(find.text('AwesomeStudioPedal'), findsOneWidget);
    expect(find.text('v0.5.0 (build 1)'), findsOneWidget);
  });

  testWidgets('hands off after min-dwell when init resolves quickly',
      (tester) async {
    var ready = false;
    await tester.pumpWidget(MaterialApp(
      home: SplashScreen(
        onReady: () => ready = true,
        timings: const SplashTimings(
          minDwell: Duration(milliseconds: 100),
          maxDwell: Duration(seconds: 5),
        ),
        initFuture: Future<void>.value(),
      ),
    ));

    await tester.pump(const Duration(milliseconds: 50));
    expect(ready, isFalse);
    await tester.pump(const Duration(milliseconds: 100));
    expect(ready, isTrue);
  });

  testWidgets('hands off at max-dwell even if init never resolves',
      (tester) async {
    var ready = false;
    final neverFinishes = Completer<void>().future;
    await tester.pumpWidget(MaterialApp(
      home: SplashScreen(
        onReady: () => ready = true,
        timings: const SplashTimings(
          minDwell: Duration(milliseconds: 100),
          maxDwell: Duration(milliseconds: 300),
        ),
        initFuture: neverFinishes,
      ),
    ));

    await tester.pump(const Duration(milliseconds: 200));
    expect(ready, isFalse);
    await tester.pump(const Duration(milliseconds: 200));
    expect(ready, isTrue);
  });

  testWidgets('reduced-motion path drops the pulse animation', (tester) async {
    await tester.pumpWidget(MaterialApp(
      home: MediaQuery(
        data: const MediaQueryData(disableAnimations: true),
        child: SplashScreen(
          onReady: () {},
          timings: const SplashTimings(
            minDwell: Duration(milliseconds: 50),
            maxDwell: Duration(milliseconds: 100),
          ),
          initFuture: Future<void>.value(),
        ),
      ),
    ));
    await tester.pump();

    expect(find.byKey(SplashScreen.staticMarkKey), findsOneWidget);
    expect(find.byKey(SplashScreen.pulseMarkKey), findsNothing);

    // Drain pending timers before tear-down.
    await tester.pump(const Duration(milliseconds: 200));
  });
}
