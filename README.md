
<p align="center">
  <img src="https://raw.githubusercontent.com/Dronecode/UX-Design/35d8148a8a0559cd4bcf50bfa2c94614983cce91/QGC/Branding/Deliverables/QGC_RGB_Logo_Horizontal_Positive_PREFERRED/QGC_RGB_Logo_Horizontal_Positive_PREFERRED.svg" alt="QGroundControl Logo" width="500">
</p>

<p align="center">
  <a href="https://github.com/mavlink/QGroundControl/releases">
    <img src="https://img.shields.io/github/release/mavlink/QGroundControl.svg" alt="Latest Release">
  </a>
</p>

*QGroundControl* (QGC) is a highly intuitive and powerful Ground Control Station (GCS) designed for UAVs. Whether you're a first-time pilot or an experienced professional, QGC provides a seamless user experience for flight control and mission planning, making it the go-to solution for any *MAVLink-enabled drone*.

> **📋 This is a customized fork of [QGroundControl](https://github.com/mavlink/qgroundcontrol) with enterprise authentication features for multi-user drone fleet management.**

---

### 🌟 *Why Choose This Version?*

- *🚀 Ease of Use*: A beginner-friendly interface designed for smooth operation without sacrificing advanced features for pros.
- *✈️ Comprehensive Flight Control*: Full flight control and mission management for *PX4* and *ArduPilot* powered UAVs.
- *🛠️ Mission Planning*: Easily plan complex missions with a simple drag-and-drop interface.
- *🔐 Enterprise Authentication*: Custom two-phase authentication system for secure multi-user drone fleet management.

🔍 For a deeper dive into using QGC, check out the [User Manual](https://docs.qgroundcontrol.com/en/) – although, thanks to QGC's intuitive UI, you may not even need it!

---

### 🚁 *Key Features*

- 🕹️ *Full Flight Control*: Supports all *MAVLink drones*.
- ⚙️ *Vehicle Setup*: Tailored configuration for *PX4* and *ArduPilot* platforms.
- 🔧 *Fully Open Source*: Customize and extend the software to suit your needs.

🎯 Check out the latest updates in our [New Features and Release Notes](https://github.com/mavlink/qgroundcontrol/blob/master/ChangeLog.md).

---

### 🔐 *Custom Authentication System*

This version includes a professional enterprise authentication framework for secure multi-user drone operations:

#### **Two-Phase Authentication Workflow**
1. **Phase 1 - User Login**: Enter username/password to authenticate with the server
2. **Drone Assignment**: System retrieves list of drones assigned to the user
3. **Phase 2 - Drone Selection**: Select desired drone and authenticate with drone-specific credentials
4. **Session Establishment**: Secure session key generated for MAVLink communication

#### **Core Components**

| Component | File | Purpose |
|-----------|------|---------|
| **AuthenticatedLink** | `src/Comms/AuthenticatedLink.h/.cc` | Two-phase authentication protocol with TCP/UDP hybrid connection |
| **DroneSelectionDialog** | `src/QmlControls/DroneSelectionDialog.qml` | Interactive UI for drone selection with status indicators |
| **AuthenticatedSettings** | `src/UI/AppSettings/AuthenticatedSettings.qml` | Connection configuration (username, password, server URL) |
| **DroneInfo** | `src/Comms/DroneInfo.h/.cc` | Rich drone metadata (firmware, hardware, permissions, camera streams) |

#### **Key Capabilities**
- ✅ Username/password-based authentication
- ✅ Automatic drone list retrieval upon login
- ✅ Visual drone selection with status indicators (ready, active, online)
- ✅ API key alternative for programmatic connections
- ✅ Hybrid protocol: TCP for authentication, UDP for MAVLink data
- ✅ Session-based communication with security validation
- ✅ Multi-user fleet management support

#### **Configuration**

**Default Server**
- TCP Authentication Port: `45.117.171.237:5760`
- UDP Data Port: `45.117.171.237:5761`
- Custom server URLs supported via Application Settings → Comm Links → Authenticated

**Connection Methods**
1. *Standard Login*: Username/Password → Drone Selection → Connect
2. *API Key Method*: Drone UUID + API Key → Direct Connection

#### **Testing & Development**

📚 Complete testing documentation and mock server setup available in:
- [DRONE_SELECTION_GUIDE.md](./DRONE_SELECTION_GUIDE.md) - Comprehensive testing guide
- `tools/mock_auth_server.py` - Local authentication server for development

**Quick Start Testing**
```bash
# Start mock authentication server
python3 tools/mock_auth_server.py

# Default test credentials
# Username: testuser
# Password: testpass
```

---

### 💻 *Get Involved!*

This is a *customized open-source* QGroundControl build, derived from the [official QGroundControl project](https://github.com/mavlink/qgroundcontrol). Whether you're fixing bugs, adding authentication features, or customizing for your specific drone fleet management needs, contributions are welcome!

🛠️ Start building today:
- [QGC Developer Guide](https://dev.qgroundcontrol.com/en/)
- [Build Instructions](https://dev.qgroundcontrol.com/en/getting_started/)
- [Authentication Testing Guide](./DRONE_SELECTION_GUIDE.md)

---

### 🔗 *Useful Links*

**Official QGC Resources**
- 🌐 [Official Website](http://qgroundcontrol.com)
- 📘 [User Manual](https://docs.qgroundcontrol.com/en/)
- 🛠️ [Developer Guide](https://dev.qgroundcontrol.com/en/)
- 💬 [Discussion & Support](https://docs.qgroundcontrol.com/en/Support/Support.html)

**Custom Version**
- 🔐 [Authentication Testing Guide](./DRONE_SELECTION_GUIDE.md)
- 📖 [Custom Authentication Architecture](./src/Comms/README.md) *(documentation coming soon)*

**Community & Contribution**
- 🤝 [Original Project Contributing](https://dev.qgroundcontrol.com/en/contribute/)
- 📄 [License: Apache 2.0 & GPL v3](./LICENSE-APACHE)

---

With QGroundControl's full flight control capabilities and enterprise authentication, you're in complete command of your UAV fleet, ready to take your missions to the next level.
