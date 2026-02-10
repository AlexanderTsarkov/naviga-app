import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:latlong2/latlong.dart';

import '../../app/app_state.dart';
import '../connect/connect_controller.dart';
import '../nodes/nodes_controller.dart';
import '../nodes/nodes_state.dart';

/// Default center when there are no markers (e.g. world view).
const LatLng _kDefaultCenter = LatLng(55.75, 37.62);
const double _kDefaultZoom = 4.0;

/// Padding for fitBounds so markers sit in ~80% inner contour.
const double _kFitBoundsPaddingFraction = 0.1;

/// Max lastSeen age (seconds) for "active nodes for map" — fitBounds uses only
/// nodes seen within this window so the map fits "nodes near me" not stale ones.
const int _kActiveMaxAgeS = 180;

class MapScreen extends ConsumerStatefulWidget {
  const MapScreen({super.key});

  @override
  ConsumerState<MapScreen> createState() => _MapScreenState();
}

class _MapScreenState extends ConsumerState<MapScreen> {
  final MapController _mapController = MapController();
  bool _initialFitApplied = false;

  @override
  Widget build(BuildContext context) {
    final nodesState = ref.watch(nodesControllerProvider);
    final mapFocusNodeId = ref.watch(mapFocusNodeIdProvider);

    final markersWithPos = _nodesWithValidPos(nodesState);
    final markersForFit = _nodesForFit(nodesState);
    final selfNodeId = nodesState.self?.nodeId;

    ref.listen<int?>(mapFocusNodeIdProvider, (prev, next) {
      if (!mounted) return;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (!mounted) return;
        final state = ref.read(nodesControllerProvider);
        final forDisplay = _nodesWithValidPos(state);
        final forFit = _nodesForFit(state);
        if (next != null) {
          final target = _findNode(forDisplay, next);
          if (target != null) {
            _mapController.move(
              LatLng(_e7ToDeg(target.latE7), _e7ToDeg(target.lonE7)),
              14,
            );
          }
        } else {
          if (forFit.isEmpty) return;
          if (kDebugMode) {
            for (final r in forFit) {
              debugPrint(
                'Map bounds marker: nodeId=${r.nodeId} '
                'lat=${_e7ToDeg(r.latE7)} lon=${_e7ToDeg(r.lonE7)}',
              );
            }
          }
          final bounds = _boundsFromMarkers(forFit);
          _mapController.fitCamera(
            CameraFit.insideBounds(
              bounds: bounds,
              padding: _paddingFromFraction(_kFitBoundsPaddingFraction),
            ),
          );
        }
      });
    });

    return FlutterMap(
      mapController: _mapController,
      options: MapOptions(
        initialCenter: _kDefaultCenter,
        initialZoom: _kDefaultZoom,
        interactionOptions: const InteractionOptions(
          flags: InteractiveFlag.all & ~InteractiveFlag.rotate,
        ),
        onMapReady: () =>
            _onMapReady(markersWithPos, markersForFit, mapFocusNodeId),
      ),
      children: [
        TileLayer(
          urlTemplate: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
          userAgentPackageName: 'app.naviga',
          tileProvider: NetworkTileProvider(),
        ),
        if (markersWithPos.isEmpty)
          const Center(
            child: Padding(
              padding: EdgeInsets.all(24),
              child: Text(
                'No nodes with position yet.\nConnect and refresh NodeTable.',
                textAlign: TextAlign.center,
              ),
            ),
          )
        else
          MarkerLayer(
            markers: markersWithPos
                .map((r) => _buildMarker(r, selfNodeId: selfNodeId))
                .toList(),
          ),
      ],
    );
  }

  /// Nodes shown as markers: pos_valid, non-zero lat/lon, sanity (lat -90..90,
  /// lon -180..180). Used for display and for centering on "Show on map" target.
  List<NodeRecordV1> _nodesWithValidPos(NodesState state) {
    return state.recordsSorted.where((r) {
      if (!r.posValid) return false;
      if (r.latE7 == 0 && r.lonE7 == 0) return false;
      final lat = _e7ToDeg(r.latE7);
      final lon = _e7ToDeg(r.lonE7);
      return _sanityLatLon(lat, lon);
    }).toList();
  }

  /// Active nodes for map fitBounds only: same as valid pos + lastSeenAgeS <= 180s.
  /// Fit uses only this list so the map fits "nodes near me" not stale ones.
  List<NodeRecordV1> _nodesForFit(NodesState state) {
    return _nodesWithValidPos(
      state,
    ).where((r) => r.lastSeenAgeS <= _kActiveMaxAgeS).toList();
  }

  NodeRecordV1? _findNode(List<NodeRecordV1> list, int nodeId) {
    for (final r in list) {
      if (r.nodeId == nodeId) return r;
    }
    return null;
  }

  static bool _sanityLatLon(double lat, double lon) {
    return lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180;
  }

  void _onMapReady(
    List<NodeRecordV1> markersWithPos,
    List<NodeRecordV1> markersForFit,
    int? mapFocusNodeId,
  ) {
    if (!mounted || _initialFitApplied) return;

    if (markersWithPos.isEmpty) {
      _initialFitApplied = true;
      return;
    }

    if (mapFocusNodeId != null) {
      final target = _findNode(markersWithPos, mapFocusNodeId);
      if (target != null) {
        _mapController.move(
          LatLng(_e7ToDeg(target.latE7), _e7ToDeg(target.lonE7)),
          14,
        );
        _initialFitApplied = true;
        return;
      }
    }

    if (markersForFit.isEmpty) {
      _initialFitApplied = true;
      return;
    }
    if (kDebugMode) {
      for (final r in markersForFit) {
        debugPrint(
          'Map bounds marker: nodeId=${r.nodeId} '
          'lat=${_e7ToDeg(r.latE7)} lon=${_e7ToDeg(r.lonE7)}',
        );
      }
    }
    final bounds = _boundsFromMarkers(markersForFit);
    final padding = _paddingFromFraction(_kFitBoundsPaddingFraction);
    _mapController.fitCamera(
      CameraFit.insideBounds(bounds: bounds, padding: padding),
    );
    _initialFitApplied = true;
  }

  LatLngBounds _boundsFromMarkers(List<NodeRecordV1> list) {
    double minLat = 90, maxLat = -90, minLon = 180, maxLon = -180;
    for (final r in list) {
      final lat = _e7ToDeg(r.latE7);
      final lon = _e7ToDeg(r.lonE7);
      if (lat < minLat) minLat = lat;
      if (lat > maxLat) maxLat = lat;
      if (lon < minLon) minLon = lon;
      if (lon > maxLon) maxLon = lon;
    }
    return LatLngBounds(LatLng(minLat, minLon), LatLng(maxLat, maxLon));
  }

  EdgeInsets _paddingFromFraction(double fraction) {
    final media = MediaQuery.sizeOf(context);
    final pad = fraction * media.shortestSide;
    return EdgeInsets.all(pad);
  }

  static double _e7ToDeg(int e7) => e7 / 1e7;

  Marker _buildMarker(NodeRecordV1 r, {int? selfNodeId}) {
    final isSelf = selfNodeId != null && r.nodeId == selfNodeId;
    return Marker(
      point: LatLng(_e7ToDeg(r.latE7), _e7ToDeg(r.lonE7)),
      width: isSelf ? 36 : 28,
      height: isSelf ? 36 : 28,
      child: Icon(
        Icons.location_on,
        size: isSelf ? 36 : 28,
        color: isSelf
            ? Theme.of(context).colorScheme.primary
            : Theme.of(context).colorScheme.outline,
      ),
    );
  }
}
