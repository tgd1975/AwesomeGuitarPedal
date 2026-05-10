import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:url_launcher/url_launcher.dart';
import '../services/app_info.dart';
import '../services/ble_service.dart';
import '../theme/asp_theme.dart';
import '../widgets/connection_details_sheet.dart';
import '../widgets/content_page_scaffold.dart';

class InfoAboutScreen extends StatelessWidget {
  const InfoAboutScreen({super.key});

  @override
  Widget build(BuildContext context) => ContentPageScaffold(
        title: 'About',
        child: FutureBuilder<AppInfo>(
          future: AppInfo.load(),
          builder: (context, snapshot) {
            final appInfo = snapshot.data;
            if (appInfo == null) {
              return const Center(child: CircularProgressIndicator());
            }
            return SingleChildScrollView(
              child: _InfoAboutContent(appInfo: appInfo),
            );
          },
        ),
      );
}

class _InfoAboutContent extends StatelessWidget {
  const _InfoAboutContent({required this.appInfo});

  final AppInfo appInfo;

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          'AwesomeStudioPedal is an open-source, BLE-configurable foot '
          'controller for musicians. Plug it into a phone, laptop, or '
          'tablet over USB or Bluetooth, then map each button to keystrokes, '
          'macros, or HID commands using this app.',
          style: textTheme.bodyLarge,
        ),
        const SizedBox(height: 24),
        Text(appInfo.versionDisplay, style: textTheme.titleLarge),
        const SizedBox(height: 4),
        Text(appInfo.firmwareCompatibilityDisplay,
            style: textTheme.labelMedium),
        const SizedBox(height: 12),
        const _ConnectionSubsection(),
        const SizedBox(height: 24),
        const _LinksSection(),
        const SizedBox(height: 24),
        Text(
          'Released under the ${AppInfo.license} License. ${AppInfo.copyright}.',
          style: textTheme.labelMedium,
        ),
      ],
    );
  }
}

class _ConnectionSubsection extends StatelessWidget {
  const _ConnectionSubsection();

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<BleService>();
    final textTheme = Theme.of(context).textTheme;
    final connected = ble.isConnected;
    return InkWell(
      onTap: () => showConnectionDetailsSheet(context),
      child: Padding(
        padding: const EdgeInsets.symmetric(vertical: 10),
        child: Row(
          children: [
            const Icon(Icons.bluetooth, color: AspTokens.accent, size: 20),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Connection', style: textTheme.bodyLarge),
                  Text(
                    connected
                        ? '${ble.deviceName ?? 'Pedal'} · ${ble.deviceId ?? '—'}'
                        : 'Not connected',
                    style: textTheme.labelMedium,
                  ),
                ],
              ),
            ),
            const Icon(Icons.chevron_right,
                size: 16, color: AspTokens.textMuted),
          ],
        ),
      ),
    );
  }
}

class _LinksSection extends StatelessWidget {
  const _LinksSection();

  @override
  Widget build(BuildContext context) => const Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _ExternalLinkTile(
            icon: Icons.code,
            label: 'GitHub repository',
            url: AppInfo.githubRepoUrl,
          ),
          _ExternalLinkTile(
            icon: Icons.public,
            label: 'Project website',
            url: AppInfo.projectSiteUrl,
          ),
          _ExternalLinkTile(
            icon: Icons.history,
            label: 'Release notes',
            url: AppInfo.releaseNotesUrl,
          ),
          _ExternalLinkTile(
            icon: Icons.bug_report,
            label: 'Report a bug',
            url: AppInfo.reportBugUrl,
          ),
        ],
      );
}

class _ExternalLinkTile extends StatelessWidget {
  const _ExternalLinkTile({
    required this.icon,
    required this.label,
    required this.url,
  });

  final IconData icon;
  final String label;
  final String url;

  @override
  Widget build(BuildContext context) => InkWell(
        onTap: () => _open(context),
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 10),
          child: Row(
            children: [
              Icon(icon, color: AspTokens.accent, size: 20),
              const SizedBox(width: 12),
              Expanded(
                child:
                    Text(label, style: Theme.of(context).textTheme.bodyLarge),
              ),
              const Icon(Icons.open_in_new,
                  size: 16, color: AspTokens.textMuted),
            ],
          ),
        ),
      );

  Future<void> _open(BuildContext context) async {
    final uri = Uri.parse(url);
    final ok = await launchUrl(uri, mode: LaunchMode.externalApplication);
    if (!ok && context.mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not open $url')),
      );
    }
  }
}
