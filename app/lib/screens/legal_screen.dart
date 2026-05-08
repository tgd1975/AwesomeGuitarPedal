import 'package:flutter/material.dart';
import '../services/app_info.dart';
import '../widgets/content_page_scaffold.dart';

class LegalScreen extends StatelessWidget {
  const LegalScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final textTheme = Theme.of(context).textTheme;
    return ContentPageScaffold(
      title: 'Legal & Open Source',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text('AwesomeStudioPedal', style: textTheme.headlineMedium),
          const SizedBox(height: 8),
          Text(
            'Released under the ${AppInfo.license} License. ${AppInfo.copyright}.',
            style: textTheme.bodyLarge,
          ),
          const SizedBox(height: 4),
          Text(
            'The firmware is licensed under the same terms.',
            style: textTheme.bodyMedium,
          ),
          const SizedBox(height: 24),
          OutlinedButton.icon(
            icon: const Icon(Icons.list_alt),
            label: const Text('View third-party licenses'),
            onPressed: () => showLicensePage(
              context: context,
              applicationName: 'AwesomeStudioPedal',
              applicationLegalese: AppInfo.copyright,
            ),
          ),
        ],
      ),
    );
  }
}
