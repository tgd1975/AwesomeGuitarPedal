import 'dart:async';
import 'dart:convert';
import 'dart:io' show Platform;
import 'dart:math';

import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

import '../constants/ble_constants.dart';
import '../models/upload_result.dart';

class BleService extends ChangeNotifier {
  BluetoothDevice? _device;
  BluetoothCharacteristic? _writeChar;
  BluetoothCharacteristic? _writeHwChar;
  BluetoothCharacteristic? _statusChar;
  BluetoothCharacteristic? _hwIdentityChar;
  StreamSubscription<BluetoothConnectionState>? _connSub;

  bool _connected = false;
  String? _error;

  bool get isConnected => _connected;
  String? get lastError => _error;

  /// Human-readable name of the currently connected device (or null if
  /// no device is connected).
  String? get deviceName => _device?.platformName;

  /// Stable identifier (Android: MAC, iOS: UUID) of the currently
  /// connected device, or null if no device is connected.
  String? get deviceId => _device?.remoteId.str;

  Stream<String> get statusStream {
    if (_statusChar == null) return const Stream.empty();
    return _statusChar!.onValueReceived.map((v) => utf8.decode(v));
  }

  Stream<BluetoothAdapterState> get adapterState =>
      FlutterBluePlus.adapterState;

  BluetoothAdapterState get adapterStateNow => FlutterBluePlus.adapterStateNow;

  /// Whether the runtime BLE permissions needed for scanning + connecting
  /// are currently granted. Android 12+ (API 31) requires BLUETOOTH_SCAN
  /// and BLUETOOTH_CONNECT; older Android required ACCESS_FINE_LOCATION
  /// for scanning. iOS / desktop have no equivalent runtime gate, so we
  /// short-circuit to true. See TASK-262.
  Future<bool> hasBlePermissions() async {
    if (kIsWeb || !Platform.isAndroid) return true;
    final results = await Future.wait([
      Permission.bluetoothScan.status,
      Permission.bluetoothConnect.status,
    ]);
    return results.every((s) => s.isGranted);
  }

  /// Triggers the system permission prompt(s) for scanning + connecting.
  /// Returns true if both are granted afterwards; false if the user
  /// denied (transiently or permanently). See TASK-262.
  Future<bool> requestBlePermissions() async {
    if (kIsWeb || !Platform.isAndroid) return true;
    final results = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
    ].request();
    return results.values.every((s) => s.isGranted);
  }

  /// Whether at least one of the BLE runtime permissions is permanently
  /// denied (the user tapped "Don't ask again" or denied twice). When
  /// true, requesting again is a no-op — the only path forward is the
  /// app's system-settings page, opened with [openAppSettings].
  Future<bool> isBlePermissionPermanentlyDenied() async {
    if (kIsWeb || !Platform.isAndroid) return false;
    final results = await Future.wait([
      Permission.bluetoothScan.status,
      Permission.bluetoothConnect.status,
    ]);
    return results.any((s) => s.isPermanentlyDenied);
  }

  Future<List<ScanResult>> scan(
      {Duration timeout = const Duration(seconds: 10)}) async {
    _clearError();

    // Permission pre-flight. On Android 12+, FlutterBluePlus.startScan
    // does not throw when BLUETOOTH_SCAN / BLUETOOTH_CONNECT are denied
    // — the platform call returns silently and no scan results arrive,
    // leaving the caller on an indefinite spinner. Detect this up front
    // and surface a "permission" error the UI can render. See TASK-262.
    if (!await hasBlePermissions()) {
      final granted = await requestBlePermissions();
      if (!granted) {
        _setError('Bluetooth permission denied');
        return [];
      }
    }

    // Adapter pre-flight. If the radio is off, FlutterBluePlus.startScan
    // does not throw — it accepts the call, the platform-side scan is
    // never started, and the wait simply elapses with empty results. The
    // user sees an indefinite spinner. Catch that here so the UI can
    // render a "Bluetooth is off" state instead. See TASK-262.
    if (FlutterBluePlus.adapterStateNow != BluetoothAdapterState.on) {
      _setError('Bluetooth is off');
      return [];
    }

    try {
      // Unfiltered scan + client-side name prefix match. We cannot use
      // withServices: [kServiceUuid] because the firmware advertises only
      // the HID UUID (0x1812), not our 128-bit config UUID — a UUID filter
      // would reject the pedal. See KNOWN_ISSUES.md and TASK-258.
      //
      // androidLegacy: true forces a 1M-PHY (legacy) scan. The pedal's
      // ESP32 NimBLE stack only does legacy advertising, and the
      // flutter_blue_plus default (`androidLegacy: false`) uses Android
      // 14+'s extended-advertising API, which on some devices (Pixel 9
      // / Android 16 verified) misses legacy-only peripherals
      // entirely — onScanResult never fires. See TASK-258.
      await FlutterBluePlus.startScan(
        timeout: timeout,
        androidLegacy: true,
      );

      // Listen for asynchronous scan failures (permission denied, adapter
      // turned off mid-scan, etc.) so they propagate as an error instead
      // of as silent empty results. Without this, denying the runtime
      // BLUETOOTH_SCAN permission produces an indefinite spinner — the
      // platform pushes the error through `scanResults.addError` but
      // `lastScanResults` and `Future.delayed` never observe it. See
      // TASK-262.
      late StreamSubscription<List<ScanResult>> errSub;
      final completer = Completer<void>();
      errSub = FlutterBluePlus.scanResults.listen(
        (_) {},
        onError: (Object e) {
          if (e is FlutterBluePlusException) {
            _setError(e.description ?? e.toString());
          } else {
            _setError(e.toString());
          }
          if (!completer.isCompleted) completer.complete();
        },
      );

      await Future.any([Future.delayed(timeout), completer.future]);
      await errSub.cancel();
      await FlutterBluePlus.stopScan();

      if (_error != null) return [];

      final lower = kPedalNamePrefix.toLowerCase();
      final seen = <String>{};
      return [
        for (final r in FlutterBluePlus.lastScanResults)
          if (r.device.platformName.toLowerCase().startsWith(lower) &&
              seen.add(r.device.remoteId.str))
            r,
      ];
    } on FlutterBluePlusException catch (e) {
      _setError(e.description ?? e.toString());
      return [];
    }
  }

  Future<void> connect(BluetoothDevice device) async {
    _clearError();
    try {
      await device.connect(autoConnect: false);
      _device = device;

      _connSub = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          _connected = false;
          _writeChar = null;
          _writeHwChar = null;
          _statusChar = null;
          _hwIdentityChar = null;
          notifyListeners();
        }
      });

      final services = await device.discoverServices();
      final pedal = services.firstWhere(
        (s) => s.uuid.str128.toLowerCase() == kServiceUuid,
        orElse: () => throw Exception('Pedal GATT service not found'),
      );

      for (final c in pedal.characteristics) {
        final uuid = c.uuid.str128.toLowerCase();
        if (uuid == kConfigWriteUuid) _writeChar = c;
        if (uuid == kConfigWriteHwUuid) _writeHwChar = c;
        if (uuid == kConfigStatusUuid) _statusChar = c;
        if (uuid == kHwIdentityUuid) _hwIdentityChar = c;
      }

      if (_statusChar != null) {
        await _statusChar!.setNotifyValue(true);
      }

      _connected = true;
      notifyListeners();
    } on FlutterBluePlusException catch (e) {
      _setError(e.description ?? e.toString());
    } catch (e) {
      _setError(e.toString());
    }
  }

  void disconnect() {
    _connSub?.cancel();
    _device?.disconnect();
    _device = null;
    _writeChar = null;
    _writeHwChar = null;
    _statusChar = null;
    _hwIdentityChar = null;
    _connected = false;
    notifyListeners();
  }

  Future<UploadResult> uploadProfiles(String json) =>
      _upload(json, _writeChar, 'CONFIG_WRITE');

  Future<UploadResult> uploadConfig(String json) =>
      _upload(json, _writeHwChar, 'CONFIG_WRITE_HW');

  /// Reads the hardware identity characteristic from the connected device.
  /// Returns the board string (e.g. "esp32" / "nrf52840"), or null if the
  /// characteristic is unavailable (old firmware).
  Future<String?> readDeviceHardware() async {
    if (_hwIdentityChar == null) return null;
    try {
      final bytes = await _hwIdentityChar!.read();
      return utf8.decode(bytes).trim();
    } on FlutterBluePlusException {
      return null;
    }
  }

  Future<UploadResult> _upload(
    String json,
    BluetoothCharacteristic? characteristic,
    String label,
  ) async {
    if (characteristic == null) {
      return const UploadResult.failure(
          'Not connected or characteristic unavailable');
    }
    if (_statusChar == null) {
      return const UploadResult.failure('Status characteristic unavailable');
    }

    final byteLength = utf8.encode(json).length;
    if (byteLength > kMaxConfigBytes) {
      return UploadResult.failure(
          'Config is $byteLength bytes — exceeds device limit of '
          '$kMaxConfigBytes bytes (${kMaxConfigBytes ~/ 1024} KB). '
          'Trim profiles or shorten action values and retry.');
    }

    final completer = Completer<UploadResult>();
    StreamSubscription<List<int>>? sub;
    sub = _statusChar!.onValueReceived.listen((value) {
      final msg = utf8.decode(value);
      sub?.cancel();
      if (msg == 'OK') {
        completer.complete(const UploadResult.success());
      } else {
        final reason = msg.startsWith('ERROR:') ? msg.substring(6) : msg;
        completer.complete(UploadResult.failure(reason));
      }
    });

    try {
      final bytes = utf8.encode(json);
      // Android caps writeWithoutResponse at MTU - 3. Without an explicit
      // requestMtu, Android negotiates MTU=255 → 252 B payload. iOS allows
      // a similar cap. 180 B + 2-byte seq header = 182 B fits comfortably
      // and stays well under any negotiated MTU on Android/iOS. See TASK-261.
      const chunkSize = 180;
      final totalChunks = (bytes.length / chunkSize).ceil();

      for (int i = 0; i < totalChunks; i++) {
        final payload = bytes.sublist(
          i * chunkSize,
          min((i + 1) * chunkSize, bytes.length),
        );
        final seq = Uint8List(2)
          ..buffer.asByteData().setUint16(0, i, Endian.big);
        await characteristic.write([...seq, ...payload], withoutResponse: true);
      }

      // End-of-transfer packet.
      final eof = Uint8List(2)
        ..buffer.asByteData().setUint16(0, 0xFFFF, Endian.big);
      await characteristic.write([...eof], withoutResponse: true);
    } catch (e) {
      // Catch-all: flutter_blue_plus_android throws PlatformException, not
      // FlutterBluePlusException, on GATT-level failures (chunk too large,
      // disconnect mid-write, MTU change). Without this broad catch the
      // exception escapes, the StreamSubscription leaks, and the caller's
      // progress UI hangs forever. See TASK-266.
      await sub.cancel();
      final reason = e is FlutterBluePlusException
          ? (e.description ?? e.toString())
          : e.toString();
      return UploadResult.failure(reason);
    }

    return completer.future.timeout(
      const Duration(seconds: 15),
      onTimeout: () {
        sub?.cancel();
        return const UploadResult.failure(
            'Upload timed out waiting for status');
      },
    );
  }

  void _setError(String message) {
    _error = message;
    notifyListeners();
  }

  void _clearError() {
    _error = null;
  }
}
