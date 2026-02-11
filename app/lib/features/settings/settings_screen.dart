import 'package:flutter/material.dart';
import 'package:package_info_plus/package_info_plus.dart';

import 'about_extended_screen.dart';
import 'legal_screen.dart';
import 'settings_about_text.dart';

/// Current map tile source (read-only). From map_screen.dart: OpenStreetMap.
const String _kMapSourceLabel = 'OpenStreetMap (online)';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  String _appVersion = '—';
  bool _diagnosticsToggle = false;

  @override
  void initState() {
    super.initState();
    _loadAppVersion();
  }

  Future<void> _loadAppVersion() async {
    try {
      final info = await PackageInfo.fromPlatform();
      if (mounted) {
        setState(() {
          _appVersion = '${info.version}+${info.buildNumber}';
        });
      }
    } catch (_) {
      // Keep "—" on failure
    }
  }

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 16),
      children: [
        _buildAboutSection(),
        const SizedBox(height: 24),
        _buildMapSourceSection(),
        const SizedBox(height: 24),
        _buildDiagnosticsSection(),
        const SizedBox(height: 24),
        _buildLegalSection(),
      ],
    );
  }

  Widget _buildAboutSection() {
    return _SectionCard(
      title: 'About',
      children: [
        _RowLabel(label: 'Version', value: _appVersion),
        const SizedBox(height: 12),
        SelectableText(
          aboutShortSummary,
          style: Theme.of(context).textTheme.bodyMedium,
        ),
        const SizedBox(height: 12),
        FilledButton.icon(
          onPressed: () {
            Navigator.of(context).push(
              MaterialPageRoute<void>(
                builder: (_) => const AboutExtendedScreen(),
              ),
            );
          },
          icon: const Icon(Icons.info_outline, size: 18),
          label: const Text('Подробнее о проекте'),
        ),
      ],
    );
  }

  Widget _buildMapSourceSection() {
    return _SectionCard(
      title: 'Map source',
      children: [
        _RowLabel(label: 'Current', value: _kMapSourceLabel),
        const SizedBox(height: 8),
        Text(
          'В будущих версиях планируется поддержка оффлайн-карт',
          style: Theme.of(context).textTheme.bodySmall?.copyWith(
            color: Theme.of(context).colorScheme.outline,
          ),
        ),
      ],
    );
  }

  Widget _buildDiagnosticsSection() {
    return _SectionCard(
      title: 'Diagnostics',
      children: [
        Row(
          children: [
            Expanded(
              child: Text(
                'Diagnostics mode (reserved for future versions)',
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: Theme.of(context).colorScheme.outline,
                ),
              ),
            ),
            Switch(
              value: _diagnosticsToggle,
              onChanged: (value) {
                setState(() => _diagnosticsToggle = value);
              },
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildLegalSection() {
    return _SectionCard(
      title: 'Legal',
      children: [
        OutlinedButton.icon(
          onPressed: () {
            Navigator.of(context).push(
              MaterialPageRoute<void>(builder: (_) => const LegalScreen()),
            );
          },
          icon: const Icon(Icons.gavel, size: 18),
          label: const Text('Ответственность пользователя'),
        ),
      ],
    );
  }
}

class _SectionCard extends StatelessWidget {
  const _SectionCard({required this.title, required this.children});

  final String title;
  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text(title, style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            ...children,
          ],
        ),
      ),
    );
  }
}

class _RowLabel extends StatelessWidget {
  const _RowLabel({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 100,
            child: Text(label, style: Theme.of(context).textTheme.titleSmall),
          ),
          Expanded(child: Text(value)),
        ],
      ),
    );
  }
}
