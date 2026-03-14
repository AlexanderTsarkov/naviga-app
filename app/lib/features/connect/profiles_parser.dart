import 'dart:typed_data';

// S04 #467: Parse BLE profiles list and profile read response payloads.
// List: n_radio(1), radio_ids(4×n LE), n_user(1), user_ids(4×n LE).
// Radio profile: profile_id(4), kind(1), channel_slot(1), rate_tier(1), tx_power_baseline_step(1), label_len(1), label(0..24).
// Role profile: role_id(4), min_interval_sec(2), max_silence_10s(1), min_displacement_m(4 float LE).

/// Parsed profiles list (radio IDs and user/role IDs).
class ParsedProfilesList {
  const ParsedProfilesList({required this.radioIds, required this.userIds});

  final List<int> radioIds;
  final List<int> userIds;
}

/// Parsed radio profile (type=0).
class ParsedRadioProfile {
  const ParsedRadioProfile({
    required this.profileId,
    required this.kind,
    required this.channelSlot,
    required this.rateTier,
    required this.txPowerBaselineStep,
    required this.label,
  });

  final int profileId;
  final int kind;
  final int channelSlot;
  final int rateTier;
  final int txPowerBaselineStep;
  final String label;
}

/// Parsed role/user profile (type=1).
class ParsedRoleProfile {
  const ParsedRoleProfile({
    required this.roleId,
    required this.minIntervalSec,
    required this.maxSilence10s,
    required this.minDisplacementM,
  });

  final int roleId;
  final int minIntervalSec;
  final int maxSilence10s;
  final double minDisplacementM;
}

/// Result of parsing one profile read response (either radio or role).
sealed class ParsedProfile {}

class ParsedProfileRadio extends ParsedProfile {
  ParsedProfileRadio(this.data);
  final ParsedRadioProfile data;
}

class ParsedProfileRole extends ParsedProfile {
  ParsedProfileRole(this.data);
  final ParsedRoleProfile data;
}

class BleProfilesParser {
  BleProfilesParser._();

  static int _readU32Le(List<int> d, int i) =>
      (d[i] & 0xFF) |
      ((d[i + 1] & 0xFF) << 8) |
      ((d[i + 2] & 0xFF) << 16) |
      ((d[i + 3] & 0xFF) << 24);

  static int _readU16Le(List<int> d, int i) =>
      (d[i] & 0xFF) | ((d[i + 1] & 0xFF) << 8);

  /// Parse list payload: n_radio(1), radio_ids(4×n), n_user(1), user_ids(4×n). Returns null if invalid.
  static ParsedProfilesList? parseList(List<int> bytes) {
    if (bytes.length < 2) return null;
    final nRadio = bytes[0] & 0xFF;
    int off = 1;
    if (bytes.length < off + nRadio * 4 + 1) return null;
    final radioIds = <int>[];
    for (var i = 0; i < nRadio; i++) {
      radioIds.add(_readU32Le(bytes, off));
      off += 4;
    }
    final nUser = bytes[off++] & 0xFF;
    if (bytes.length < off + nUser * 4) return null;
    final userIds = <int>[];
    for (var i = 0; i < nUser; i++) {
      userIds.add(_readU32Le(bytes, off));
      off += 4;
    }
    return ParsedProfilesList(radioIds: radioIds, userIds: userIds);
  }

  /// Parse profile read response. [type] 0=radio, 1=role. Returns null if invalid.
  static ParsedProfile? parseProfileResponse(int type, List<int> bytes) {
    if (type == 0) {
      if (bytes.length < 10) return null;
      final profileId = _readU32Le(bytes, 0);
      final kind = bytes[4] & 0xFF;
      final channelSlot = bytes[5] & 0xFF;
      final rateTier = bytes[6] & 0xFF;
      final txPowerBaselineStep = bytes[7] & 0xFF;
      final labelLen = (bytes[8] & 0xFF).clamp(0, 24);
      final label = bytes.length >= 9 + labelLen
          ? String.fromCharCodes(bytes.sublist(9, 9 + labelLen))
          : '';
      return ParsedProfileRadio(
        ParsedRadioProfile(
          profileId: profileId,
          kind: kind,
          channelSlot: channelSlot,
          rateTier: rateTier,
          txPowerBaselineStep: txPowerBaselineStep,
          label: label,
        ),
      );
    }
    if (type == 1) {
      if (bytes.length < 11) return null;
      final roleId = _readU32Le(bytes, 0);
      final minIntervalSec = _readU16Le(bytes, 4);
      final maxSilence10s = bytes[6] & 0xFF;
      final floatBits = _readU32Le(bytes, 7);
      final minDisplacementM = _uint32ToFloat(floatBits);
      return ParsedProfileRole(
        ParsedRoleProfile(
          roleId: roleId,
          minIntervalSec: minIntervalSec,
          maxSilence10s: maxSilence10s,
          minDisplacementM: minDisplacementM,
        ),
      );
    }
    return null;
  }

  static double _uint32ToFloat(int bits) {
    final buffer = ByteData(4);
    buffer.setUint32(0, bits, Endian.little);
    return buffer.getFloat32(0, Endian.little);
  }
}
