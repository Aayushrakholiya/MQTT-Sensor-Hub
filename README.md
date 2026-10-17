<div align="center">

# 📡 MQTT Sensor Hub

**An IoT sensing + communication platform built for SENG3030: M5StickCPlus2 devices publish live environmental sensor data over secured MQTT, while a Python desktop control panel monitors every device in the class and manages subscriptions in real time.**

ESP32 · MQTT over TLS · M5StickC · Python Tkinter

<br>

![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?style=flat-square&logo=esphome&logoColor=white)
![MQTT](https://img.shields.io/badge/MQTT_TLS-660066?style=flat-square&logo=mqtt&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B_Arduino-00979D?style=flat-square&logo=arduino&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=flat-square&logo=python&logoColor=white)
![Tkinter](https://img.shields.io/badge/Tkinter_UI-blue?style=flat-square)

</div>

---

## 📌 Table of Contents

- [About](#-about)
- [The Device Firmware](#-the-device-firmware)
- [MQTT Topic Layout](#-mqtt-topic-layout)
- [The Control Panel](#-the-control-panel)
- [Project Structure](#%EF%B8%8F-project-structure)
- [Getting Started](#-getting-started)
- [Team](#-team)

## 👋 About

Two halves make the hub:

1. **M5Stick_Mic** — PlatformIO firmware for an M5StickCPlus2 that reads ENV sensors, IMU motion, battery level, and even records/plays raw audio, publishing everything to an EMQX Cloud broker over **MQTT over TLS (port 8883)**
2. **controlPanel** — a Python Tkinter desktop app that subscribes to the whole class topic space (`SENG3030/#`), discovers every device that's publishing, shows each device's live topics, and lets you subscribe/unsubscribe to specific feeds with day-agnostic filters

## 🎛️ The Device Firmware

The M5StickCPlus2 packs a surprising amount of sensing into a matchbox:

| Stream | What it publishes |
| --- | --- |
| SHT40 | Temperature + humidity (I2C, external port) |
| BMP280 | Temperature + barometric pressure (I2C, external port) |
| IMU | Accelerometer + gyroscope readings — **shake detection** built in |
| Battery | Live battery voltage in mV |
| Microphone | Records raw 16-bit PCM audio and publishes it in chunks — other sticks receive and play it back through their speakers |
| UI | On-device screen shows live readings; button toggles sending on/off |

The firmware connects over WiFi to an EMQX Cloud broker with username/password auth and a pinned DigiCert root CA for TLS.

## 📬 MQTT Topic Layout

```text
SENG3030/Thursday/<username>/
├── battery              Battery voltage (mV)
├── bmp280/temperature   BMP280 temperature
├── bmp280/pressure      BMP280 pressure
├── sht40/temperature    SHT40 temperature
├── sht40/humidity       SHT40 humidity
├── audio/raw            Raw audio payload
├── mic/meta             Sample rate + chunk metadata
└── mic/bytes            Raw 16-bit PCM audio chunks (play on receive)
```

## 🖥️ The Control Panel

The Python control panel is the operator's view of the whole network:

- **Auto-discovery** — watches every message under `SENG3030/#` and groups devices by the username in their topic
- **Device browser** — pick a device, see all of its live topics
- **Subscribe / unsubscribe** — send subscription commands to a chosen device for any topic filter
- **Day-agnostic filters** — `SENG3030/Thursday/...` automatically becomes `SENG3030/+/...` so filters keep working across class days
- **Live message log** with topic filters

## 🗂️ Project Structure

```text
MQTT-Sensor-Hub/
├── M5Stick_Mic/              🎛️ PlatformIO firmware (M5StickCPlus2)
│   ├── platformio.ini        Build config + libs (M5Unified, M5Unit-ENV, PubSubClient)
│   └── src/main.cpp          Sensors, IMU, mic record/play, MQTT client
├── controlPanel/
│   └── controlPanel.py       🖥️ Tkinter MQTT monitor + subscription manager
└── arakholiya_team.pdf       📄 Project report
```

## 🚀 Getting Started

**Device firmware:**

```bash
# install PlatformIO, then from M5Stick_Mic/
pio run --target upload        # flash the M5StickCPlus2
pio device monitor             # watch serial output
```

Update WiFi credentials and MQTT username/password in `src/main.cpp` first.

**Control panel:**

```bash
pip install paho-mqtt
python controlPanel/controlPanel.py
```

Update `MQTT_BROKER`, `MQTT_USERNAME`, and `MQTT_PASSWORD` at the top of the script.

## 👥 Team

Built by **Ayushkumar Rakholiya, Darsh Patel, Viraj Solanki & Jal Shah** (SENG3030 project).

---

<div align="center">

⭐ **Star this repo if you find it useful** ⭐

</div>
