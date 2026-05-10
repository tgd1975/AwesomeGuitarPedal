import 'package:awesome_studio_pedal/screens/info_about_screen.dart';
import 'package:awesome_studio_pedal/services/app_info.dart';
import 'package:awesome_studio_pedal/services/ble_service.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:mockito/annotations.dart';
import 'package:mockito/mockito.dart';
import 'package:provider/provider.dart';

@GenerateNiceMocks([MockSpec<BleService>()])
import 'info_about_screen_test.mocks.dart';

Widget _wrap(Widget child) {
  final ble = MockBleService();
  when(ble.isConnected).thenReturn(false);
  when(ble.deviceName).thenReturn(null);
  when(ble.deviceId).thenReturn(null);
  return ChangeNotifierProvider<BleService>.value(
    value: ble,
    child: MaterialApp(home: child),
  );
}

void main() {
  setUp(() {
    AppInfo.overrideForTesting(
      AppInfo.forTesting(version: '0.4.2', buildNumber: '7'),
    );
  });

  tearDown(() {
    AppInfo.resetForTesting();
  });

  testWidgets('renders version, pitch, license, and four external links',
      (tester) async {
    await tester.pumpWidget(_wrap(const InfoAboutScreen()));
    await tester.pumpAndSettle();

    expect(find.text('About'), findsOneWidget);
    expect(find.text('v0.4.2 (build 7)'), findsOneWidget);
    expect(find.textContaining('open-source'), findsOneWidget);
    expect(find.textContaining('MIT License'), findsOneWidget);
    expect(find.textContaining('Compatible with firmware'), findsOneWidget);

    expect(find.text('GitHub repository'), findsOneWidget);
    expect(find.text('Project website'), findsOneWidget);
    expect(find.text('Release notes'), findsOneWidget);
    expect(find.text('Report a bug'), findsOneWidget);
  });

  testWidgets('fits a Pixel 9 form factor without overflow', (tester) async {
    await binding.setSurfaceSize(const Size(412, 915));
    addTearDown(() => binding.setSurfaceSize(null));

    await tester.pumpWidget(_wrap(const InfoAboutScreen()));
    await tester.pumpAndSettle();

    expect(tester.takeException(), isNull);
  });

  testWidgets('tapping a link invokes the url_launcher platform channel',
      (tester) async {
    final calls = <MethodCall>[];
    const channel = MethodChannel('plugins.flutter.io/url_launcher');
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      if (call.method == 'canLaunch') return true;
      if (call.method == 'launch') return true;
      return null;
    });
    addTearDown(() {
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
          .setMockMethodCallHandler(channel, null);
    });

    await tester.pumpWidget(_wrap(const InfoAboutScreen()));
    await tester.pumpAndSettle();

    await tester.tap(find.text('GitHub repository'));
    await tester.pump();

    final urls = calls
        .map((c) => (c.arguments as Map?)?['url'] as String?)
        .whereType<String>()
        .toList();
    expect(urls, contains(AppInfo.githubRepoUrl));
  });
}

TestWidgetsFlutterBinding get binding =>
    TestWidgetsFlutterBinding.ensureInitialized();
