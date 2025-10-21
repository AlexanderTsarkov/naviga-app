# Naviga - Autonomous Outdoor Navigation System

**Naviga** is an autonomous navigation and communication solution designed for outdoor activities in areas without cellular coverage. The system combines Meshtastic mesh networking devices with offline maps to provide reliable communication and navigation for hunters, hikers, and wilderness explorers.

## 🎯 Key Features

- **Offline Operation**: Works completely without cellular or internet connectivity
- **Mesh Networking**: Uses Meshtastic devices for peer-to-peer communication
- **Offline Maps**: Pre-loaded maps for specific regions (Russia, Estonia)
- **Real-time Tracking**: GPS position sharing between group members
- **Geo-objects**: Share waypoints, hunting spots, and danger zones
- **Cross-platform**: Android and iOS support

## 🗺️ Target Regions

- Novgorod Oblast, Russia
- Rostov Oblast, Russia  
- Yaroslavl Oblast, Russia
- Estonia

## 🛠️ Technology Stack

- **Mobile App**: Flutter (Android/iOS)
- **Hardware**: T-beam LoRa devices with Meshtastic firmware
- **Maps**: OpenStreetMap with offline tile caching
- **Communication**: Bluetooth + LoRa mesh networking

## 📱 Project Status

Currently in **POC (Proof of Concept)** phase, focusing on:
- ✅ Meshtastic device integration
- 🔄 Offline map implementation
- ⏳ Basic mesh communication
- ⏳ Field testing in target regions

## 🚀 Getting Started

### Prerequisites
- Flutter SDK
- Android Studio / Xcode
- Meshtastic T-beam device

### Installation
```bash
git clone https://github.com/AlexanderTsarkov/naviga-app.git
cd naviga-app
flutter pub get
flutter run
```

## 📋 Development Plan

See [POC Implementation Plan](docs/План%20реализации%20POC.md) for detailed development roadmap.

## 🤝 Contributing

This project is currently in active development. Contact the team for contribution guidelines.

## 📄 License

[License information to be added]

## 👥 Team

- **Alexander** - Mobile app development
- **Mikhail** - Hardware and firmware (Estonia)

---

*Built for outdoor enthusiasts who need reliable communication and navigation in remote areas.*
