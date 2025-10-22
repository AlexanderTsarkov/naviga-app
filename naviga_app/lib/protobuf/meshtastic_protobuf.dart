import 'dart:typed_data';

/// Простые protobuf классы для Meshtastic на основе официальной документации
/// Основано на https://buf.build/meshtastic/protobufs
class ToRadio {
  int? wantConfigId;
  MeshPacket? packet;
  
  Uint8List writeToBuffer() {
    final buffer = <int>[];
    
    if (wantConfigId != null) {
      // ToRadio.want_config_id = <nonce> (поле 3, а не 1!)
      buffer.addAll([0x18]); // field 3, wire type 0 (varint)
      buffer.addAll(_encodeVarint(wantConfigId!));
    }
    
    if (packet != null) {
      // ToRadio.packet = <MeshPacket> (поле 1)
      buffer.addAll([0x0A]); // field 1, wire type 2 (length-delimited)
      final packetBytes = packet!.writeToBuffer();
      buffer.addAll(_encodeVarint(packetBytes.length));
      buffer.addAll(packetBytes);
    }
    
    return Uint8List.fromList(buffer);
  }
}

class FromRadio {
  MeshPacket? packet;
  MyNodeInfo? myInfo;
  int? configCompleteId;
  
  static FromRadio fromBuffer(Uint8List data) {
    final fromRadio = FromRadio();
    int offset = 0;
    
    while (offset < data.length) {
      final tag = data[offset++];
      final fieldNumber = tag >> 3;
      final wireType = tag & 0x07;
      
      switch (fieldNumber) {
        case 1: // packet
          if (wireType == 2) { // length-delimited
            final length = _decodeVarint(data, offset);
            offset += length.bytesRead;
            final packetData = data.sublist(offset, offset + length.value);
            offset += length.value;
            fromRadio.packet = MeshPacket.fromBuffer(packetData);
          }
          break;
        case 2: // my_info
          if (wireType == 2) { // length-delimited
            final length = _decodeVarint(data, offset);
            offset += length.bytesRead;
            final myInfoData = data.sublist(offset, offset + length.value);
            offset += length.value;
            fromRadio.myInfo = MyNodeInfo.fromBuffer(myInfoData);
          }
          break;
        case 3: // config_complete_id
          if (wireType == 0) { // varint
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            fromRadio.configCompleteId = result.value;
          }
          break;
      }
    }
    
    return fromRadio;
  }
}

class MeshPacket {
  int? to;
  int? from;
  int? channel;
  Data? decoded;
  
  Uint8List writeToBuffer() {
    final buffer = <int>[];
    
    if (to != null) {
      buffer.addAll([0x08]); // field 1
      buffer.addAll(_encodeVarint(to!));
    }
    
    if (from != null) {
      buffer.addAll([0x10]); // field 2
      buffer.addAll(_encodeVarint(from!));
    }
    
    if (channel != null) {
      buffer.addAll([0x18]); // field 3
      buffer.addAll(_encodeVarint(channel!));
    }
    
    if (decoded != null) {
      buffer.addAll([0x22]); // field 4
      final decodedBytes = decoded!.writeToBuffer();
      buffer.addAll(_encodeVarint(decodedBytes.length));
      buffer.addAll(decodedBytes);
    }
    
    return Uint8List.fromList(buffer);
  }
  
  static MeshPacket fromBuffer(Uint8List data) {
    final packet = MeshPacket();
    int offset = 0;
    
    while (offset < data.length) {
      final tag = data[offset++];
      final fieldNumber = tag >> 3;
      final wireType = tag & 0x07;
      
      switch (fieldNumber) {
        case 1: // to
          if (wireType == 0) {
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            packet.to = result.value;
          }
          break;
        case 2: // from
          if (wireType == 0) {
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            packet.from = result.value;
          }
          break;
        case 3: // channel
          if (wireType == 0) {
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            packet.channel = result.value;
          }
          break;
        case 4: // decoded
          if (wireType == 2) {
            final length = _decodeVarint(data, offset);
            offset += length.bytesRead;
            final decodedData = data.sublist(offset, offset + length.value);
            offset += length.value;
            packet.decoded = Data.fromBuffer(decodedData);
          }
          break;
      }
    }
    
    return packet;
  }
}

class Data {
  int? portnum;
  Uint8List? payload;
  
  Uint8List writeToBuffer() {
    final buffer = <int>[];
    
    if (portnum != null) {
      buffer.addAll([0x08]); // field 1
      buffer.addAll(_encodeVarint(portnum!));
    }
    
    if (payload != null) {
      buffer.addAll([0x12]); // field 2
      buffer.addAll(_encodeVarint(payload!.length));
      buffer.addAll(payload!);
    }
    
    return Uint8List.fromList(buffer);
  }
  
  static Data fromBuffer(Uint8List data) {
    final dataObj = Data();
    int offset = 0;
    
    while (offset < data.length) {
      final tag = data[offset++];
      final fieldNumber = tag >> 3;
      final wireType = tag & 0x07;
      
      switch (fieldNumber) {
        case 1: // portnum
          if (wireType == 0) {
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            dataObj.portnum = result.value;
          }
          break;
        case 2: // payload
          if (wireType == 2) {
            final length = _decodeVarint(data, offset);
            offset += length.bytesRead;
            dataObj.payload = data.sublist(offset, offset + length.value);
            offset += length.value;
          }
          break;
      }
    }
    
    return dataObj;
  }
}

class MyNodeInfo {
  int? myNodeNum;
  bool? hasGps;
  int? numChannels;
  
  static MyNodeInfo fromBuffer(Uint8List data) {
    final myInfo = MyNodeInfo();
    int offset = 0;
    
    while (offset < data.length) {
      final tag = data[offset++];
      final fieldNumber = tag >> 3;
      final wireType = tag & 0x07;
      
      switch (fieldNumber) {
        case 1: // my_node_num
          if (wireType == 0) {
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            myInfo.myNodeNum = result.value;
          }
          break;
        case 2: // has_gps
          if (wireType == 0) {
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            myInfo.hasGps = result.value != 0;
          }
          break;
        case 3: // num_channels
          if (wireType == 0) {
            final result = _decodeVarint(data, offset);
            offset += result.bytesRead;
            myInfo.numChannels = result.value;
          }
          break;
      }
    }
    
    return myInfo;
  }
}

// Вспомогательные функции для protobuf
List<int> _encodeVarint(int value) {
  final buffer = <int>[];
  while (value >= 0x80) {
    buffer.add((value & 0xFF) | 0x80);
    value >>= 7;
  }
  buffer.add(value & 0xFF);
  return buffer;
}

class VarintResult {
  final int value;
  final int bytesRead;
  VarintResult(this.value, this.bytesRead);
}

VarintResult _decodeVarint(Uint8List data, int offset) {
  int value = 0;
  int shift = 0;
  int bytesRead = 0;
  
  while (offset + bytesRead < data.length) {
    final byte = data[offset + bytesRead];
    bytesRead++;
    
    value |= (byte & 0x7F) << shift;
    
    if ((byte & 0x80) == 0) {
      break;
    }
    
    shift += 7;
  }
  
  return VarintResult(value, bytesRead);
}
