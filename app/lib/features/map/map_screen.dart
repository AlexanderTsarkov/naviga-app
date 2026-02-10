import 'dart:async';
import 'dart:math' show cos, max;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:latlong2/latlong.dart';

import '../../app/app_state.dart';
import '../../shared/app_tabs.dart';
import '../connect/connect_controller.dart';
import '../nodes/nodes_controller.dart';
import '../nodes/nodes_state.dart';

/// Default center when there are no markers (e.g. world view).
const LatLng _kDefaultCenter = LatLng(55.75, 37.62);
const double _kDefaultZoom = 4.0;

/// Padding for fitBounds so markers sit in ~80% inner contour; at least this many px.
const double _kFitBoundsPaddingFraction = 0.1;
const double _kMinPaddingPx = 96.0;

/// Extra zoom-out applied after fitBounds (2+ points) so both nodes stay visible.
const double _kFitZoomOutMargin = 0.75;

/// Max lastSeen age (seconds) for "active nodes for map" — fitBounds uses only
/// nodes seen within this window so the map fits "nodes near me" not stale ones.
/// Kept short (25s) to avoid fitting to ocean-scale when stale/outlier nodes exist.
const int _kActiveMaxAgeS = 25;

/// Zoom for single-point view and max zoom when fitting bounds.
const double _kSinglePointZoom = 17.0;
const double _kFitMaxZoom = 17.0;
const double _kFitMinZoom = 3.0;

/// Debounce delay so only the latest scheduleFit applies (avoids tabOpen/onMapReady/focus races).
const int _kScheduleFitDelayMs = 250;

class MapScreen extends ConsumerStatefulWidget {
  const MapScreen({super.key});

  @override
  ConsumerState<MapScreen> createState() => _MapScreenState();
}

class _MapScreenState extends ConsumerState<MapScreen> {
  final MapController _mapController = MapController();
  bool _initialFitApplied = false;
  int _scheduleFitGeneration = 0;
  Timer? _scheduleFitTimer;

  @override
  Widget build(BuildContext context) {
    final nodesState = ref.watch(nodesControllerProvider);

    final markersWithPos = _nodesWithValidPos(nodesState);
    final markersForFit = _nodesForFit(nodesState);
    final selfNodeId = nodesState.self?.nodeId;

    ref.listen<int?>(mapFocusNodeIdProvider, (prev, next) {
      if (!mounted) return;
      _scheduleFit(next != null ? 'focusSet' : 'focusCleared');
    });
    ref.listen<AppTab>(selectedTabProvider, (prev, current) {
      if (!mounted) return;
      if (current == AppTab.map && ref.read(mapFocusNodeIdProvider) == null) {
        _scheduleFit('tabOpen');
      }
    });

    return FlutterMap(
      mapController: _mapController,
      options: MapOptions(
        initialCenter: _kDefaultCenter,
        initialZoom: _kDefaultZoom,
        interactionOptions: const InteractionOptions(
          flags: InteractiveFlag.all & ~InteractiveFlag.rotate,
        ),
        onMapReady: () => _onMapReady(markersWithPos, markersForFit),
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

  @override
  void dispose() {
    _scheduleFitTimer?.cancel();
    super.dispose();
  }

  void _onMapReady(
    List<NodeRecordV1> markersWithPos,
    List<NodeRecordV1> markersForFit,
  ) {
    if (!mounted || _initialFitApplied) return;
    if (markersWithPos.isEmpty) {
      _initialFitApplied = true;
      return;
    }
    _initialFitApplied = true;
    _scheduleFit('firstOpen');
  }

  /// Schedules fit/center with debounce; only the latest schedule runs (avoids
  /// races between tabOpen/onMapReady/focusCleared/focusSet).
  void _scheduleFit(String reason) {
    _scheduleFitTimer?.cancel();
    final generation = ++_scheduleFitGeneration;
    _scheduleFitTimer = Timer(
      const Duration(milliseconds: _kScheduleFitDelayMs),
      () {
        if (!mounted) return;
        if (generation != _scheduleFitGeneration) return;
        WidgetsBinding.instance.addPostFrameCallback((_) {
          if (!mounted) return;
          if (generation != _scheduleFitGeneration) return;
          _runFit(reason);
        });
      },
    );
  }

  /// Runs fit/center; only when Map tab is visible; no-op if active list empty.
  void _runFit(String reason) {
    final selectedTab = ref.read(selectedTabProvider);
    if (selectedTab != AppTab.map) return;
    final state = ref.read(nodesControllerProvider);
    final forDisplay = _nodesWithValidPos(state);
    final forFit = _nodesForFit(state);
    final focusId = ref.read(mapFocusNodeIdProvider);

    if (focusId != null) {
      final target = _findNode(forDisplay, focusId);
      if (target != null) {
        _mapController.move(
          LatLng(_e7ToDeg(target.latE7), _e7ToDeg(target.lonE7)),
          14,
        );
      }
      if (kDebugMode) {
        debugPrint(
          'MAPFIT reason=$reason count=${forFit.length} action=centerFocus',
        );
      }
      return;
    }

    if (forFit.isEmpty) return;

    final bounds = _boundsFromMarkers(forFit);
    final center = bounds.center;
    final spanM = _boundsSpanMeters(bounds);
    final padding = _paddingWithMinimum(_kFitBoundsPaddingFraction);

    if (forFit.length == 1) {
      _mapController.move(center, _kSinglePointZoom);
      if (kDebugMode) {
        debugPrint(
          'MAPFIT reason=$reason count=1 action=move17 '
          'center=${center.latitude.toStringAsFixed(5)},${center.longitude.toStringAsFixed(5)} spanM=${spanM.toStringAsFixed(0)}',
        );
      }
    } else {
      _mapController.fitCamera(
        CameraFit.insideBounds(
          bounds: bounds,
          padding: padding,
          maxZoom: _kFitMaxZoom,
          minZoom: _kFitMinZoom,
        ),
      );
      final camera = _mapController.camera;
      final zoomOut = (camera.zoom - _kFitZoomOutMargin).clamp(
        _kFitMinZoom,
        _kFitMaxZoom,
      );
      _mapController.move(camera.center, zoomOut);
      if (kDebugMode) {
        final paddingPx = padding.left.toInt();
        debugPrint(
          'MAPFIT reason=$reason count=${forFit.length} action=fitBounds '
          'paddingPx=$paddingPx appliedZoomOutMargin=$_kFitZoomOutMargin '
          'south=${bounds.south.toStringAsFixed(5)} west=${bounds.west.toStringAsFixed(5)} '
          'north=${bounds.north.toStringAsFixed(5)} east=${bounds.east.toStringAsFixed(5)} spanM=${spanM.toStringAsFixed(0)}',
        );
      }
    }
  }

  /// Approximate bounds span in meters: max of lat span and lon span at center.
  static double _boundsSpanMeters(LatLngBounds bounds) {
    const metersPerDegLat = 111320.0;
    final metersLat = (bounds.north - bounds.south) * metersPerDegLat;
    const degToRad = 0.017453292519943295; // pi / 180
    final centerLatRad = (bounds.south + bounds.north) / 2 * degToRad;
    final metersLon =
        (bounds.east - bounds.west) * metersPerDegLat * cos(centerLatRad);
    return max(metersLat.abs(), metersLon.abs());
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

  EdgeInsets _paddingWithMinimum(double fraction) {
    final media = MediaQuery.sizeOf(context);
    final pad = (fraction * media.shortestSide).clamp(
      _kMinPaddingPx,
      double.infinity,
    );
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
