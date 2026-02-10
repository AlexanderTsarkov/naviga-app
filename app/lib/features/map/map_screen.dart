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
    final selfNodeId = nodesState.self?.nodeId;

    ref.listen<int?>(mapFocusNodeIdProvider, (prev, next) {
      if (!mounted || markersWithPos.isEmpty) return;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (!mounted) return;
        if (next != null) {
          final target = _findNode(markersWithPos, next);
          if (target != null) {
            _mapController.move(
              LatLng(_e7ToDeg(target.latE7), _e7ToDeg(target.lonE7)),
              14,
            );
          }
        } else {
          final state = ref.read(nodesControllerProvider);
          final list = state.recordsSorted
              .where((r) => r.posValid && _hasValidLatLon(r))
              .toList();
          if (list.isEmpty) return;
          final bounds = _boundsFromMarkers(list);
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
        onMapReady: () => _onMapReady(markersWithPos, mapFocusNodeId),
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

  List<NodeRecordV1> _nodesWithValidPos(NodesState state) {
    return state.recordsSorted
        .where((r) => r.posValid && _hasValidLatLon(r))
        .toList();
  }

  NodeRecordV1? _findNode(List<NodeRecordV1> list, int nodeId) {
    for (final r in list) {
      if (r.nodeId == nodeId) return r;
    }
    return null;
  }

  bool _hasValidLatLon(NodeRecordV1 r) {
    return r.latE7 != 0 || r.lonE7 != 0;
  }

  void _onMapReady(List<NodeRecordV1> markersWithPos, int? mapFocusNodeId) {
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

    final bounds = _boundsFromMarkers(markersWithPos);
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
