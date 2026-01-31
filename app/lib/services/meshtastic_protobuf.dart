import 'dart:typed_data';

/// Простой класс для создания protobuf сообщений Meshtastic
/// Основан на официальных определениях с https://buf.build/meshtastic/protobufs
class MeshtasticProtobuf {
  
  /// Создает startConfig сообщение для запроса конфигурации
  /// Согласно официальной документации, это должно быть AdminMessage с want_config_id = 1
  static Uint8List createStartConfig() {
    // ToRadio с AdminMessage want_config_id = 1
    // Структура: ToRadio { want_config_id: AdminMessage { get_config: true } }
    return Uint8List.fromList([
      0x0A, 0x02, 0x08, 0x01,  // ToRadio.want_config_id = AdminMessage.get_config = true
    ]);
  }
  
  /// Создает запрос GPS данных
  static Uint8List createGpsRequest() {
    // Запрос GPS данных через AdminMessage
    return Uint8List.fromList([
      0x0A, 0x02, 0x08, 0x01,  // AdminMessage.get_config = true
    ]);
  }
  
  /// Создает запрос информации об узле
  static Uint8List createNodeInfoRequest() {
    // Запрос информации об узле
    return Uint8List.fromList([
      0x0A, 0x00,  // ToRadio.want_config_id = AdminMessage (пустой)
    ]);
  }
  
  /// Парсит входящие данные от FromRadio
  static Map<String, dynamic>? parseFromRadioData(Uint8List data) {
    if (data.isEmpty) return null;
    
    try {
      print('🔍 Парсинг protobuf данных: ${data.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ')}');
      
      // Простой парсинг protobuf (упрощенная версия)
      if (data.length >= 1) {
        final firstByte = data[0];
        
        // Проверяем тип сообщения по первому байту
        if (firstByte == 0x0A) {
          // AdminMessage
          return _parseAdminMessage(data);
        } else if (firstByte == 0x12) {
          // MeshPacket
          return _parseMeshPacket(data);
        } else if (firstByte == 0x18) {
          // end_config (bool)
          return {'type': 'end_config', 'value': data.length > 1 ? data[1] == 1 : false};
        } else {
          print('❓ Неизвестный тип сообщения: $firstByte');
          return {'type': 'unknown', 'data': data};
        }
      }
    } catch (e) {
      print('❌ Ошибка парсинга protobuf: $e');
    }
    
    return null;
  }
  
  static Map<String, dynamic>? _parseAdminMessage(Uint8List payload) {
    // Простой парсинг AdminMessage
    if (payload.length >= 2) {
      final fieldType = payload[1];
      
      switch (fieldType) {
        case 0x08: // get_config
          return {'type': 'admin_get_config', 'value': payload.length > 2 ? payload[2] == 1 : false};
        case 0x12: // user
          return {'type': 'admin_user', 'data': payload.sublist(2)};
        case 0x1A: // radio_config
          return {'type': 'admin_radio_config', 'data': payload.sublist(2)};
        case 0x22: // my_node
          return {'type': 'admin_my_node', 'data': payload.sublist(2)};
        case 0x2A: // node_info
          return {'type': 'admin_node_info', 'data': payload.sublist(2)};
        default:
          return {'type': 'admin_unknown', 'data': payload};
      }
    }
    return {'type': 'admin_empty', 'data': payload};
  }
  
  static Map<String, dynamic>? _parseMeshPacket(Uint8List payload) {
    // Простой парсинг MeshPacket
    return {'type': 'mesh_packet', 'data': payload};
  }
}
