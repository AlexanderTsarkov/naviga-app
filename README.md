# Naviga - Autonomous Outdoor Navigation System

**Naviga** is an autonomous navigation and communication solution designed for outdoor activities in areas without cellular coverage. The system combines Meshtastic mesh networking devices with offline maps to provide reliable communication and navigation for hunters, hikers, and wilderness explorers.

## 🎯 Key Features

- **Offline Operation**: Works completely without cellular or internet connectivity
- **Mesh Networking**: Uses Meshtastic devices for peer-to-peer communication
- **Offline Maps**: Pre-loaded maps for specific regions (Russia, Estonia)
- **Real-time Tracking**: GPS position sharing between group members
- **Geo-objects**: Share waypoints, hunting spots, and danger zones
- **Cross-platform**: Android and iOS (Flutter), Web app, Backend

## 🗺️ Target Regions

- Novgorod Oblast, Russia
- Rostov Oblast, Russia  
- Yaroslavl Oblast, Russia
- Estonia

## 📁 Repository structure (Variant B)

```
Naviga/
├── app/           # Mobile app (Flutter, iOS + Android)
├── firmware/      # Firmware (see docs/firmware)
├── backend/       # Backend
├── web/           # Web app
├── docs/          # Documentation (firmware/, mobile-app/, backend/, web/, design/, adr/)
├── tools/         # Scripts and utilities
└── README.md
```

Documentation: **Start here → [docs/START_HERE.md](docs/START_HERE.md)**. See also [docs/CLEAN_SLATE.md](docs/CLEAN_SLATE.md).

## 🛠️ Technology Stack

- **Mobile App**: Flutter (Android/iOS)
- **Hardware**: T-beam LoRa devices with Meshtastic firmware
- **Maps**: OpenStreetMap with offline tile caching
- **Communication**: Bluetooth + LoRa mesh networking

## 📱 Project Status

Transition from POC to product development; repository reorganized for clean start (see [docs/CLEAN_SLATE.md](docs/CLEAN_SLATE.md)).

## 🚀 Getting Started

### Prerequisites
- Flutter SDK (for app)
- Android Studio / Xcode
- Meshtastic T-beam device (optional, for field use)

### Mobile app
```bash
git clone <repo-url>
cd Naviga/app
flutter pub get
flutter run
```

### Firmware
See [docs/firmware/](docs/firmware/) and [docs/REFERENCE_REPOS.md](docs/REFERENCE_REPOS.md).

## 🤝 Contributing

This project is currently in active development. Contact the team for contribution guidelines.

## 📄 License

[License information to be added]

## 👥 Team

- **Alexander** - Mobile app development
- **Mikhail** - Hardware and firmware (Estonia)

---

*Built for outdoor enthusiasts who need reliable communication and navigation in remote areas.*
