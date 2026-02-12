class StatusData {
  const StatusData({
    required this.formatVer,
    required this.gnss,
    required this.raw,
  });

  final int formatVer;
  final GnssStatusTlv? gnss;
  final List<int> raw;
}

class GnssStatusTlv {
  const GnssStatusTlv({
    required this.gnssState,
    required this.posValid,
    required this.posAgeS,
  });

  final int gnssState;
  final bool posValid;
  final int posAgeS;

  String get stateLabel {
    switch (gnssState) {
      case 1:
        return 'FIX_2D';
      case 2:
        return 'FIX_3D';
      case 0:
      default:
        return 'NO_FIX';
    }
  }
}

class StatusParseResult {
  const StatusParseResult({required this.data, this.warning});

  final StatusData? data;
  final String? warning;
}

class BleStatusParser {
  static const int _kHeaderBytes = 4;
  static const int _kTypeGnss = 1;

  static StatusParseResult parse(List<int> bytes) {
    if (bytes.length < _kHeaderBytes) {
      return const StatusParseResult(
        data: null,
        warning: 'Status payload too short',
      );
    }

    final formatVer = bytes[0];
    String? warning;
    if (formatVer != 1) {
      warning = 'Unknown Status format_ver=$formatVer';
    }

    GnssStatusTlv? gnss;
    var i = _kHeaderBytes;
    while (i + 2 <= bytes.length) {
      final type = bytes[i];
      final len = bytes[i + 1];
      i += 2;

      if (i + len > bytes.length) {
        warning = warning ?? 'Status TLV malformed (truncated)';
        break;
      }

      if (type == _kTypeGnss && len >= 4) {
        final gnssState = bytes[i];
        final posValid = bytes[i + 1] != 0;
        final posAgeS = bytes[i + 2] | (bytes[i + 3] << 8);
        gnss = GnssStatusTlv(
          gnssState: gnssState,
          posValid: posValid,
          posAgeS: posAgeS,
        );
      }

      i += len;
    }

    return StatusParseResult(
      data: StatusData(
        formatVer: formatVer,
        gnss: gnss,
        raw: List<int>.from(bytes),
      ),
      warning: warning,
    );
  }
}
