import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:provider/provider.dart';
import '../constants/profile_constants.dart';
import '../models/profile.dart';
import '../models/profiles_state.dart';
import '../services/file_service.dart';
import '../services/schema_service.dart';
import '../widgets/validation_banner.dart';

class ProfileListScreen extends StatelessWidget {
  const ProfileListScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<ProfilesState>();
    return Scaffold(
      appBar: AppBar(
        title: const Text('Profiles'),
        actions: [
          IconButton(
            icon: const Icon(Icons.file_upload_outlined),
            tooltip: 'Import JSON',
            onPressed: () => _import(context),
          ),
          IconButton(
            icon: const Icon(Icons.share),
            tooltip: 'Export JSON',
            onPressed: () => _export(context),
          ),
          IconButton(
            icon: const Icon(Icons.data_object),
            tooltip: 'JSON Preview',
            onPressed: () => context.push('/json-preview'),
          ),
          PopupMenuButton<String>(
            tooltip: 'More actions',
            onSelected: (v) {
              if (v == 'import_config') _importConfig(context);
              if (v == 'independent_actions') {
                context.push('/independent-actions');
              }
            },
            itemBuilder: (_) => const [
              PopupMenuItem(
                value: 'import_config',
                child: ListTile(
                  leading: Icon(Icons.settings_input_component),
                  title: Text('Import Hardware Config'),
                  contentPadding: EdgeInsets.zero,
                ),
              ),
              PopupMenuItem(
                value: 'independent_actions',
                child: ListTile(
                  leading: Icon(Icons.star_border),
                  title: Text('Independent actions'),
                  subtitle:
                      Text('Fire on every button event regardless of profile'),
                  contentPadding: EdgeInsets.zero,
                ),
              ),
            ],
          ),
        ],
      ),
      body: Column(
        children: [
          const ValidationBanner(),
          Expanded(
            child: state.profiles.isEmpty
                ? const Center(
                    child: Text('No profiles yet. Tap + to add one.'))
                : ReorderableListView.builder(
                    itemCount: state.profiles.length,
                    onReorder: (oldIndex, newIndex) {
                      if (newIndex > oldIndex) newIndex--;
                      state.reorderProfiles(oldIndex, newIndex);
                    },
                    itemBuilder: (context, i) {
                      final profile = state.profiles[i];
                      return ListTile(
                        key: ValueKey(i),
                        title: Text(profile.name),
                        subtitle: profile.description != null
                            ? Text(profile.description!)
                            : null,
                        trailing: Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            IconButton(
                              icon: const Icon(Icons.delete_outline),
                              tooltip: 'Delete profile ${profile.name}',
                              onPressed: () => state.removeProfile(i),
                            ),
                            Semantics(
                              label: 'Reorder profile ${profile.name}',
                              child: const Icon(Icons.drag_handle),
                            ),
                          ],
                        ),
                        onTap: () => context.push('/profile/$i'),
                      );
                    },
                  ),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: state.profiles.length >= kMaxProfiles
            ? null
            : () => _addProfile(context),
        tooltip: state.profiles.length >= kMaxProfiles
            ? 'Maximum $kMaxProfiles profiles reached'
            : 'Add profile',
        child: const Icon(Icons.add),
      ),
    );
  }

  void _addProfile(BuildContext context) {
    final state = context.read<ProfilesState>();
    if (state.profiles.length >= kMaxProfiles) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Maximum $kMaxProfiles profiles reached'),
        ),
      );
      return;
    }
    showDialog<void>(
      context: context,
      builder: (ctx) => _NewProfileDialog(state: state),
    );
  }

  Future<void> _import(BuildContext context) async {
    final fileService = context.read<FileService>();
    final schema = context.read<SchemaService>();
    final result = await fileService.importProfiles(schema);
    if (!context.mounted) return;
    if (!result.success) {
      await showDialog<void>(
        context: context,
        builder: (_) => AlertDialog(
          title: const Text('Import Failed'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: result.errors.map((e) => Text('• $e')).toList(),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('OK'),
            ),
          ],
        ),
      );
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Profiles imported')),
      );
    }
  }

  Future<void> _export(BuildContext context) async {
    final fileService = context.read<FileService>();
    await fileService.exportProfiles();
  }

  Future<void> _importConfig(BuildContext context) async {
    final fileService = context.read<FileService>();
    final schema = context.read<SchemaService>();
    final result = await fileService.importHardwareConfig(schema);
    if (!context.mounted) return;
    if (!result.success) {
      await showDialog<void>(
        context: context,
        builder: (_) => AlertDialog(
          title: const Text('Import Failed'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: result.errors.map((e) => Text('• $e')).toList(),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('OK'),
            ),
          ],
        ),
      );
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Hardware config loaded')),
      );
    }
  }
}

class _NewProfileDialog extends StatefulWidget {
  const _NewProfileDialog({required this.state});

  final ProfilesState state;

  @override
  State<_NewProfileDialog> createState() => _NewProfileDialogState();
}

class _NewProfileDialogState extends State<_NewProfileDialog> {
  final _formKey = GlobalKey<FormState>();
  final _nameCtrl = TextEditingController();
  final _descCtrl = TextEditingController();

  @override
  void dispose() {
    _nameCtrl.dispose();
    _descCtrl.dispose();
    super.dispose();
  }

  void _submit() {
    if (!_formKey.currentState!.validate()) return;
    widget.state.addProfile(Profile(
      name: _nameCtrl.text.trim(),
      description: _descCtrl.text.trim().isEmpty ? null : _descCtrl.text.trim(),
      buttons: {},
    ));
    Navigator.of(context).pop();
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: const Text('New Profile'),
      content: Form(
        key: _formKey,
        autovalidateMode: AutovalidateMode.onUserInteraction,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextFormField(
              controller: _nameCtrl,
              decoration: const InputDecoration(labelText: 'Name'),
              autofocus: true,
              validator: (v) =>
                  (v == null || v.trim().isEmpty) ? 'Name is required' : null,
              onFieldSubmitted: (_) => _submit(),
            ),
            TextFormField(
              controller: _descCtrl,
              decoration:
                  const InputDecoration(labelText: 'Description (optional)'),
            ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: const Text('Cancel'),
        ),
        FilledButton(onPressed: _submit, child: const Text('Add')),
      ],
    );
  }
}
