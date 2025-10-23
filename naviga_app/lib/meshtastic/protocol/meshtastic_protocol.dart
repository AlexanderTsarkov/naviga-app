import 'dart:typed_data';
import '../../generated/meshtastic/mesh.pb.dart';
import '../../generated/meshtastic/admin.pb.dart';
import '../../generated/meshtastic/portnums.pbenum.dart';
import '../../generated/meshtastic/telemetry.pb.dart';

/// Модуль для работы с Meshtastic протоколом
class MeshtasticProtocol {
  
  /// Создание запроса want_config_id
  ToRadio createWantConfigIdRequest() {
    final nonce = DateTime.now().millisecondsSinceEpoch;
    return ToRadio()..wantConfigId = nonce;
  }

  /// Создание ToRadio сообщения с пакетом
  ToRadio createToRadioMessage(MeshPacket packet) {
    return ToRadio()..packet = packet;
  }

  /// Создание запроса позиции
  MeshPacket createPositionRequest(int nodeNum) {
    final queryPos = Position(); // Пустой Position = запрос позиции
    final data = Data()
      ..portnum = PortNum.POSITION_APP
      ..payload = queryPos.writeToBuffer()
      ..wantResponse = true;
    
    return MeshPacket()
      ..from = 0
      ..to = nodeNum
      ..channel = 1 // Naviga channel
      ..decoded = data
      ..hopLimit = 3;
  }

  /// Создание запроса телеметрии
  MeshPacket createTelemetryRequest(int nodeNum) {
    final queryTelemetry = Telemetry(); // Пустой Telemetry = запрос телеметрии
    final data = Data()
      ..portnum = PortNum.TELEMETRY_APP
      ..payload = queryTelemetry.writeToBuffer()
      ..wantResponse = true;
    
    return MeshPacket()
      ..from = 0
      ..to = nodeNum
      ..channel = 1 // Naviga channel
      ..decoded = data
      ..hopLimit = 3;
  }

  /// Создание запроса информации о узле
  MeshPacket createNodeInfoRequest(int nodeNum) {
    final queryUser = User(); // Пустой User = запрос информации
    final data = Data()
      ..portnum = PortNum.NODEINFO_APP
      ..payload = queryUser.writeToBuffer()
      ..wantResponse = true;
    
    return MeshPacket()
      ..from = 0
      ..to = nodeNum
      ..channel = 1 // Naviga channel
      ..decoded = data
      ..hopLimit = 3;
  }

  /// Парсинг FromRadio сообщения
  FromRadio parseFromRadio(Uint8List data) {
    return FromRadio.fromBuffer(data);
  }

  /// Парсинг Position данных
  Position parsePosition(Uint8List payload) {
    return Position()..mergeFromBuffer(payload);
  }

  /// Парсинг Telemetry данных
  Telemetry parseTelemetry(Uint8List payload) {
    return Telemetry()..mergeFromBuffer(payload);
  }

  /// Парсинг User данных
  User parseUser(Uint8List payload) {
    return User()..mergeFromBuffer(payload);
  }

  /// Парсинг NodeInfo данных
  NodeInfo parseNodeInfo(Uint8List payload) {
    return NodeInfo()..mergeFromBuffer(payload);
  }

  /// Создание текстового сообщения
  MeshPacket createTextMessage(String text, {int? toNodeNum}) {
    final data = Data()
      ..portnum = PortNum.TEXT_MESSAGE_APP
      ..payload = Uint8List.fromList(text.codeUnits)
      ..wantResponse = false;
    
    return MeshPacket()
      ..from = 0
      ..to = toNodeNum ?? 0xFFFFFFFF // Broadcast
      ..channel = 0 // Primary channel
      ..decoded = data
      ..hopLimit = 3;
  }

  /// Создание сообщения с геообъектом
  MeshPacket createGeoObjectMessage({
    required double latitude,
    required double longitude,
    required String name,
    required String description,
    int? toNodeNum,
  }) {
    // Создаем JSON с геообъектом
    final geoObject = {
      'type': 'geo_object',
      'latitude': latitude,
      'longitude': longitude,
      'name': name,
      'description': description,
      'timestamp': DateTime.now().millisecondsSinceEpoch,
    };
    
    final jsonString = geoObject.toString();
    final data = Data()
      ..portnum = PortNum.TEXT_MESSAGE_APP
      ..payload = Uint8List.fromList(jsonString.codeUnits)
      ..wantResponse = false;
    
    return MeshPacket()
      ..from = 0
      ..to = toNodeNum ?? 0xFFFFFFFF // Broadcast
      ..channel = 1 // Naviga channel
      ..decoded = data
      ..hopLimit = 3;
  }

  /// Проверка валидности GPS координат
  bool isValidGpsCoordinates(double latitude, double longitude) {
    return latitude >= -90 && latitude <= 90 && 
           longitude >= -180 && longitude <= 180;
  }

  /// Конвертация координат в формат Meshtastic (int32 * 1e7)
  int latitudeToInt32(double latitude) {
    return (latitude * 10000000).round();
  }

  /// Конвертация координат в формат Meshtastic (int32 * 1e7)
  int longitudeToInt32(double longitude) {
    return (longitude * 10000000).round();
  }

  /// Конвертация из формата Meshtastic в обычные координаты
  double int32ToLatitude(int latitudeI) {
    return latitudeI / 10000000.0;
  }

  /// Конвертация из формата Meshtastic в обычные координаты
  double int32ToLongitude(int longitudeI) {
    return longitudeI / 10000000.0;
  }
}
