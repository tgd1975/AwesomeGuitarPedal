import 'package:awesome_studio_pedal/services/ble_service.dart';
import 'package:awesome_studio_pedal/widgets/connection_details_sheet.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:mockito/annotations.dart';
import 'package:mockito/mockito.dart';
import 'package:provider/provider.dart';

@GenerateNiceMocks([MockSpec<BleService>()])
import 'connection_details_sheet_test.mocks.dart';

void main() {
  testWidgets('renders all rows; missing fields show "—"', (tester) async {
    final ble = MockBleService();
    when(ble.isConnected).thenReturn(false);
    when(ble.deviceName).thenReturn(null);
    when(ble.deviceId).thenReturn(null);

    await tester.pumpWidget(
      ChangeNotifierProvider<BleService>.value(
        value: ble,
        child: const MaterialApp(
          home: Scaffold(body: ConnectionDetailsSheet()),
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(find.text('Connection details'), findsOneWidget);
    expect(find.text('Device'), findsOneWidget);
    expect(find.text('Identifier'), findsOneWidget);
    expect(find.text('Firmware'), findsOneWidget);
    expect(find.text('Signal'), findsOneWidget);
    // 4 em-dashes, one per row.
    expect(find.text('—'), findsNWidgets(4));
  });

  testWidgets('connected: shows fields and Disconnect button', (tester) async {
    final ble = MockBleService();
    when(ble.isConnected).thenReturn(true);
    when(ble.deviceName).thenReturn('AwesomePedal');
    when(ble.deviceId).thenReturn('AA:BB:CC');

    await tester.pumpWidget(
      ChangeNotifierProvider<BleService>.value(
        value: ble,
        child: const MaterialApp(
          home: Scaffold(body: ConnectionDetailsSheet()),
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(find.text('AwesomePedal'), findsOneWidget);
    expect(find.text('AA:BB:CC'), findsOneWidget);
    expect(find.text('Disconnect'), findsOneWidget);
  });

  testWidgets('Disconnect button calls BleService.disconnect once',
      (tester) async {
    final ble = MockBleService();
    when(ble.isConnected).thenReturn(true);
    when(ble.deviceName).thenReturn('AwesomePedal');
    when(ble.deviceId).thenReturn('AA');

    await tester.pumpWidget(
      ChangeNotifierProvider<BleService>.value(
        value: ble,
        child: const MaterialApp(
          home: Scaffold(body: ConnectionDetailsSheet()),
        ),
      ),
    );
    await tester.pumpAndSettle();

    await tester.tap(find.text('Disconnect'));
    await tester.pumpAndSettle();

    verify(ble.disconnect()).called(1);
  });
}
