import 'package:awesome_studio_pedal/services/ble_service.dart';
import 'package:awesome_studio_pedal/widgets/connection_status_strip.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:mockito/annotations.dart';
import 'package:mockito/mockito.dart';
import 'package:provider/provider.dart';

@GenerateNiceMocks([MockSpec<BleService>()])
import 'connection_status_strip_test.mocks.dart';

void main() {
  testWidgets('disconnected state renders "Not connected"', (tester) async {
    final ble = MockBleService();
    when(ble.isConnected).thenReturn(false);
    when(ble.deviceName).thenReturn(null);
    when(ble.deviceId).thenReturn(null);

    await tester.pumpWidget(
      ChangeNotifierProvider<BleService>.value(
        value: ble,
        child: const MaterialApp(
          home: Scaffold(body: ConnectionStatusStrip()),
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(find.text('Not connected'), findsOneWidget);
  });

  testWidgets('connected state renders name + short id', (tester) async {
    final ble = MockBleService();
    when(ble.isConnected).thenReturn(true);
    when(ble.deviceName).thenReturn('AwesomePedal');
    when(ble.deviceId).thenReturn('AA:BB:CC:DD:EE:FF');

    await tester.pumpWidget(
      ChangeNotifierProvider<BleService>.value(
        value: ble,
        child: const MaterialApp(
          home: Scaffold(body: ConnectionStatusStrip()),
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(find.textContaining('AwesomePedal'), findsOneWidget);
    expect(find.textContaining('EE:FF'), findsOneWidget);
  });

  testWidgets('tapping the strip opens the details sheet', (tester) async {
    final ble = MockBleService();
    when(ble.isConnected).thenReturn(false);
    when(ble.deviceName).thenReturn(null);
    when(ble.deviceId).thenReturn(null);

    await tester.pumpWidget(
      ChangeNotifierProvider<BleService>.value(
        value: ble,
        child: const MaterialApp(
          home: Scaffold(body: ConnectionStatusStrip()),
        ),
      ),
    );
    await tester.pumpAndSettle();

    await tester.tap(find.byType(ConnectionStatusStrip));
    await tester.pumpAndSettle();

    expect(find.text('Connection details'), findsOneWidget);
  });
}
