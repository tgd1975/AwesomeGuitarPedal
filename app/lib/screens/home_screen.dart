import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:provider/provider.dart';
import '../services/ble_service.dart';
import '../theme/asp_theme.dart';
import '../widgets/connection_status_strip.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<BleService>();
    return Scaffold(
      appBar: AppBar(
        title: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Image.asset(
              'assets/branding/asp-mark-512.png',
              height: 32,
              width: 32,
              fit: BoxFit.contain,
            ),
            const SizedBox(width: 12),
            const Flexible(
              child: Text(
                'AwesomeStudioPedal',
                overflow: TextOverflow.ellipsis,
              ),
            ),
          ],
        ),
        actions: [
          PopupMenuButton<String>(
            tooltip: 'More',
            icon: const Icon(Icons.more_vert),
            onSelected: (value) => context.push(value),
            itemBuilder: (_) => const [
              PopupMenuItem(value: '/how-to', child: Text('How to use')),
              PopupMenuItem(
                value: '/profiles-explainer',
                child: Text('What are profiles?'),
              ),
              PopupMenuItem(
                value: '/live-keystrokes',
                child: Text('Live keystrokes'),
              ),
              PopupMenuItem(
                value: '/troubleshooting',
                child: Text('Troubleshooting'),
              ),
              PopupMenuItem(value: '/legal', child: Text('Legal')),
              PopupMenuItem(value: '/info', child: Text('About')),
            ],
          ),
        ],
      ),
      body: Column(
        children: [
          const ConnectionStatusStrip(),
          Expanded(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  _ActionCard(
                    icon: Icons.bluetooth,
                    title: 'Connect to pedal',
                    subtitle: ble.isConnected ? 'Connected' : 'Not connected',
                    color: ble.isConnected ? AspTokens.success : null,
                    onTap: () => context.push('/scan'),
                  ),
                  const SizedBox(height: 12),
                  _ActionCard(
                    icon: Icons.tune,
                    title: 'Edit profiles',
                    subtitle: 'Create and edit button profiles',
                    onTap: () => context.push('/profiles'),
                  ),
                  const SizedBox(height: 12),
                  _ActionCard(
                    icon: Icons.people,
                    title: 'Community Profiles',
                    subtitle: 'Browse and load shared profile sets',
                    onTap: () => context.push('/community-profiles'),
                  ),
                  const SizedBox(height: 12),
                  _ActionCard(
                    icon: Icons.upload,
                    title: 'Upload',
                    subtitle: 'Send profiles to the pedal via BLE',
                    enabled: ble.isConnected,
                    onTap: () => context.push('/upload'),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _ActionCard extends StatelessWidget {
  const _ActionCard({
    required this.icon,
    required this.title,
    required this.subtitle,
    required this.onTap,
    this.color,
    this.enabled = true,
  });

  final IconData icon;
  final String title;
  final String subtitle;
  final VoidCallback onTap;
  final Color? color;
  final bool enabled;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Card(
      child: ListTile(
        leading: Icon(icon,
            color: color ??
                (enabled ? theme.colorScheme.primary : AspTokens.textMuted)),
        title: Text(title),
        subtitle: Text(subtitle),
        trailing: enabled ? const Icon(Icons.chevron_right) : null,
        enabled: enabled,
        onTap: enabled ? onTap : null,
      ),
    );
  }
}
