/* 
 *  FILE          : main.cpp
 *  PROJECT       : SENG3030_Project
 *  PROGRAMMER    : Ayushkumar Rakholiya, Darsh Patel, Viraj Solanki, Jal Shah
 *  FIRST VERSION : 2025-11-25
 *  DESCRIPTION   :
 *    This file contains the main code to read data from M5Unit-ENV sensors (SHT40 and BMP280).
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <string.h>
#include <M5UnitENV.h>

// I2C Pins for the external I2C port
#define EXT_SDA 32
#define EXT_SCL 33

// WiFi and MQTT Settings
const char* ssid = "Insane Asylum";
const char* password = "Humsathsathhein";
std::string mqtt_username = "Vsolanki1864";
std::string mqtt_password = "1864";
const char *mqtt_broker = "p07da41d.ala.us-east-1.emqxsl.com";
std::string control_owner_username = "Vsolanki1864";

// MQTT State Variables
String currentTopic;
String currentPayload;
bool topicLocked     = false;
bool showLiveMessage = false;
bool wasShaking      = false;

const int mqtt_port = 8883;

// MQTT Topics
const std::string mqtt_base         = "SENG3030/Thursday/" + mqtt_username + "/";
const std::string mqtt_battery      = mqtt_base + "battery";
const std::string mqtt_bmp_temperature = mqtt_base + "bmp280/temperature";
const std::string mqtt_bmp_pressure     = mqtt_base + "bmp280/pressure";
const std::string mqtt_sht_temperature  = mqtt_base + "sht40/temperature";
const std::string mqtt_sht_humidity     = mqtt_base + "sht40/humidity";
const std::string mqtt_audio           = mqtt_base + "audio/raw";

bool sendingEnabled = false;

// SHT40 Humidity and Temperature I2C sensor
SHT4X sht4;

// BMP280 Pressure and Temperature I2C sensor
BMP280 bmp;

// Flag that ENV was set up
bool env_ok = false;

void setup_unit_env(void);
void read_unit_env(void);
void publishSensorSnapshot();

WiFiClientSecure espClient;
PubSubClient client(espClient);

// MQTT publish buffer
char mqtt_buffer[512];

// Sensor readings
float sht_temp = 0.0f;
float sht_hum  = 0.0f;
float bmp_temp = 0.0f;
float bmp_pres = 0.0f;
int   battery_mv = 0;

String imu_accel = "";
String imu_gyro  = "";
static int page  = 0;

// MQTT Topics for microphone data
const std::string mqtt_mic_meta  = "SENG3030/Thursday/" + mqtt_username + "/mic/meta";
const std::string mqtt_mic_bytes = "SENG3030/Thursday/" + mqtt_username + "/mic/bytes";

const char *ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

/*---------------------------------------------------------
*  FUNCTION      : setup_wifi
*  DESCRIPTION   :
*    Sets up the WiFi connection using the provided SSID and password.
*  PARAMETERS    : None
*  RETURNS       : None
*---------------------------------------------------------*/
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/*---------------------------------------------------------
*  FUNCTION      : connectToMQTTBroker
*  DESCRIPTION   :
*    Connects to the MQTT broker and subscribes to relevant topics.
*  PARAMETERS    : None
*  RETURNS       : None
*---------------------------------------------------------*/
void connectToMQTTBroker() {

  while (!client.connected()) {

    String client_id = "esp32-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());

    if (client.connect(client_id.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
      Serial.println("Connected to MQTT broker");

      client.subscribe((mqtt_base + "#").c_str());

      client.subscribe((control_owner_username + "/" + mqtt_username + "/#").c_str());

      client.publish((mqtt_base + "hello").c_str(), "Hello SENG3030!");

      if (client.connected()) {
        int n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", sht_temp);

        if (n > 0) client.publish(mqtt_sht_temperature.c_str(), mqtt_buffer);

        n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", sht_hum);
        if (n > 0) client.publish(mqtt_sht_humidity.c_str(), mqtt_buffer);

        n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", bmp_temp);
        if (n > 0) client.publish(mqtt_bmp_temperature.c_str(), mqtt_buffer);

        n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", bmp_pres);
        if (n > 0) client.publish(mqtt_bmp_pressure.c_str(), mqtt_buffer);

        n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%d", battery_mv);
        if (n > 0) client.publish(mqtt_battery.c_str(), mqtt_buffer);

        if (imu_accel.length() > 0) {
          client.publish((mqtt_base + "imu/accel").c_str(), imu_accel.c_str());
        }
        if (imu_gyro.length() > 0) {
          client.publish((mqtt_base + "imu/gyro").c_str(), imu_gyro.c_str());
        }
      }
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(25000);
    }
  }
}

/*---------------------------------------------------------
*  FUNCTION      : setup_unit_env
*  DESCRIPTION   :
*    Sets up the M5Unit-ENV sensors (SHT40 and BMP280) over I2C.
*  PARAMETERS    : None
*  RETURNS       : None
*---------------------------------------------------------*/
void setup_unit_env(void) {

  Serial.println("* Setting Up Unit ENV *");

  if (!sht4.begin(&Wire, SHT40_I2C_ADDR_44, EXT_SDA, EXT_SCL, 400000U)) {
    Serial.println("Couldn't find SHT4x");
    while (1) {
      delay(1);
    }
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  if (!bmp.begin(&Wire, BMP280_I2C_ADDR, EXT_SDA, EXT_SCL, 400000U)) {
    Serial.println("Couldn't find BMP280");
    while (1) {
      delay(1);
    }
  }

  bmp.setSampling(BMP280::MODE_NORMAL, BMP280::SAMPLING_X2, BMP280::SAMPLING_X16, BMP280::FILTER_X16, BMP280::STANDBY_MS_500);
  env_ok = true;
}

/*---------------------------------------------------------
*  FUNCTION      : read_unit_env
*  DESCRIPTION   :
*    Reads data from the M5Unit-ENV sensors and prints to Serial.
*  PARAMETERS    : None
*  RETURNS       : None
*---------------------------------------------------------*/
void read_unit_env(void) {

  if (!env_ok) {
    Serial.println("** Unit ENV not set up **");
    return;
  }

  if (sht4.update()) {
    sht_temp = sht4.cTemp;
    sht_hum  = sht4.humidity;
    Serial.println("-----SHT4X-----");
    Serial.print("Temperature: ");
    Serial.print(sht_temp);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(sht_hum);
    Serial.println(" % rH");
    Serial.println("-------------\r\n");
  }

  if (bmp.update()) {
    bmp_temp = bmp.cTemp;
    bmp_pres = bmp.pressure;
    Serial.println("-----BMP280-----");
    Serial.print(F("Temperature: "));
    Serial.print(bmp_temp);
    Serial.println(" C");
    Serial.print(F("Pressure: "));
    Serial.print(bmp_pres);
    Serial.println(" Pa");
    Serial.println("-------------\r\n");
  }
}

// Audio recording/playback parameters
static constexpr const size_t record_number     = 200;
static constexpr const size_t record_length     = 240;
static constexpr const size_t record_size       = record_number * record_length;
static constexpr const size_t record_samplerate = 44100;

// Microphone chunk size for MQTT publishing
static constexpr const size_t MIC_CHUNK_SIZE    = 512;

// Audio recording/playback buffers and indices
static int16_t prev_y[record_length];
static int16_t prev_h[record_length];
static size_t  rec_record_idx  = 2;
static size_t  draw_record_idx = 0;
static int16_t *rec_data;

/*---------------------------------------------------------
*  FUNCTION      : mqttCallback
*  DESCRIPTION   :
*    Callback function for handling incoming MQTT messages.
*  PARAMETERS    :
*    - topic: The topic of the incoming message.
*    - payload: The payload of the incoming message.
*    - length: The length of the payload.
*  RETURNS       : None
*---------------------------------------------------------*/
void mqttCallback(char *topic, byte *payload, unsigned int length) {

  String topicStr(topic);

  if (topicStr.endsWith("/mic/bytes")) {
    Serial.println("Received /mic/bytes, playing audio...");

    if (M5.Mic.isEnabled()) {
      M5.Mic.end();
    }
    M5.Speaker.begin();

    int sampleCount = length / sizeof(int16_t);
    const int16_t *audioData = reinterpret_cast<const int16_t *>(payload);

    M5.Speaker.playRaw(audioData, sampleCount, record_samplerate, false, 1, 0);

    while (M5.Speaker.isPlaying()) {
      delay(1);
      M5.update();
    }

    M5.Speaker.end();
    M5.Mic.begin();
  }

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (topicStr.endsWith("/subscribe")) {
    Serial.print("Control SUBSCRIBE to filter: ");
    Serial.println(message);

    if (client.connected()) {
      client.subscribe(message.c_str());
    }
    return;
  } 
  else if (topicStr.endsWith("/unsubscribe")) {
    Serial.print("Control UNSUBSCRIBE from filter: ");
    Serial.println(message);

    if (client.connected()) {
      client.unsubscribe(message.c_str());
    }
    return;
  }

  Serial.print("Message received on topic: ");
  Serial.println(topicStr);
  Serial.print("Message: ");
  Serial.println(message);
  Serial.println("-----------------------");

  if (topicStr.endsWith("sht40/temperature")) {
    sht_temp = message.toFloat();
  } else if (topicStr.endsWith("sht40/humidity")) {
    sht_hum = message.toFloat();
  } else if (topicStr.endsWith("bmp280/temperature")) {
    bmp_temp = message.toFloat();
  } else if (topicStr.endsWith("bmp280/pressure")) {
    bmp_pres = message.toFloat();
  } else if (topicStr.endsWith("battery")) {
    battery_mv = message.toInt();
  } else if (topicStr.endsWith("imu/accel")) {
    imu_accel = message;
  } else if (topicStr.endsWith("imu/gyro")) {
    imu_gyro = message;
  }

  if (!topicLocked || topicStr == currentTopic) {
    currentTopic    = topicStr;
    currentPayload  = message;
    showLiveMessage = true;
    topicLocked     = true;
  }
}

/*---------------------------------------------------------
*  FUNCTION      : getShortTopic
*  DESCRIPTION   :
*    Extracts the short topic name from a full MQTT topic string.
*  PARAMETERS    :
*    - full: The full MQTT topic string.
*  RETURNS       :
*    - The short topic name as a String.
*---------------------------------------------------------*/
String getShortTopic(const String &full) {
  int lastSlash = full.lastIndexOf('/');
  if (lastSlash >= 0 && lastSlash < full.length() - 1) {
    return full.substring(lastSlash + 1);
  }
  return full;
}

/*---------------------------------------------------------
*  FUNCTION      : setup
*  DESCRIPTION   :
*    Main setup function for the M5Stack device, initializing
*    display, audio, WiFi, and MQTT connections.
*  PARAMETERS    : None
*  RETURNS       : None
*---------------------------------------------------------*/
void setup() {
  Serial.begin(115200);
  Serial.println("* Starting Setup *");

  auto cfg = M5.config();
  M5.begin(cfg);

  if (!M5.Imu.isEnabled()) {
    M5.Imu.begin();
  }

  M5.Display.fillScreen(BLUE); 

  setup_unit_env();

  M5.Display.startWrite();
  M5.Display.setRotation(1);
  M5.Display.setTextDatum(top_center);
  M5.Display.setTextColor(WHITE);
  M5.Display.setFont(&fonts::FreeSansBoldOblique12pt7b);

  rec_data = (typeof(rec_data))heap_caps_malloc(record_size * sizeof(int16_t), MALLOC_CAP_8BIT);

  if (rec_data == nullptr) {
    Serial.println("Failed to allocate rec_data buffer!");
  } else {
    memset(rec_data, 0, record_size * sizeof(int16_t));
  }

  M5.Speaker.setVolume(255);

  M5.Speaker.end();
  M5.Mic.begin();

  M5.Display.fillCircle(70, 15, 8, RED);
  M5.Display.drawString("REC", 120, 3);

setup_wifi();

espClient.setCACert(ca_cert);

client.setServer(mqtt_broker, mqtt_port);

client.setBufferSize(1024);

client.setKeepAlive(60);
client.setCallback(mqttCallback);
connectToMQTTBroker();  
}

/*---------------------------------------------------------
*  FUNCTION      : loop
*  DESCRIPTION   :
*    Main loop function for the M5Stack device, handling
*    sensor reading, MQTT communication, and user input.
*  PARAMETERS    : None
*  RETURNS       : None
*---------------------------------------------------------*/
void loop(void) {
  M5.update();
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);

  float mag2      = ax * ax + ay * ay + az * az;
  bool  isShaking = (mag2 > 1.4f);  

  if (isShaking && !wasShaking) {
    topicLocked      = false;
    showLiveMessage  = false; 

    Serial.print("Shake detected, mag2 = ");
    Serial.println(mag2);

    bool micWasOn = M5.Mic.isEnabled();
    if (micWasOn) {
      M5.Mic.end();
    }

    M5.Speaker.begin();
    M5.Speaker.tone(2000, 80);   
    delay(100);
    M5.Speaker.end();

    if (micWasOn) {
      M5.Mic.begin();
    }
  }

  wasShaking = isShaking;

  if (isShaking && !wasShaking) {
    topicLocked    = false;
    showLiveMessage = false;

    bool micWasOn = M5.Mic.isEnabled();
    if (micWasOn) {
      M5.Mic.end();
    }

    M5.Speaker.begin();
    M5.Speaker.tone(2000, 80);   
    delay(100);
    M5.Speaker.end();

    if (micWasOn) {
      M5.Mic.begin();
    }
  }

  wasShaking = isShaking;

  if (M5.BtnPWR.wasClicked()) {
    sendingEnabled = !sendingEnabled;
    if (sendingEnabled) {
      M5.Display.fillScreen(GREEN); 
    } else {
      M5.Display.fillScreen(BLUE); 
    }
  }

  if (!client.connected()) {
    connectToMQTTBroker();
  }
  client.loop();

  static unsigned long last_snapshot_ms = 0;
  const unsigned long  snapshot_interval_ms = 1000; 
  unsigned long now = millis();

  if (now - last_snapshot_ms >= snapshot_interval_ms) {
    last_snapshot_ms = now;

    read_unit_env();

    battery_mv = M5.Power.getBatteryVoltage();

    float ax2, ay2, az2, gx2, gy2, gz2;
    M5.Imu.getAccel(&ax2, &ay2, &az2);
    M5.Imu.getGyro(&gx2, &gy2, &gz2);

    imu_accel = String("AX: ") + String(ax2, 2) +
                " AY: " + String(ay2, 2) +
                " AZ: " + String(az2, 2);

    imu_gyro = String("GX: ") + String(gx2, 2) +
               " GY: " + String(gy2, 2) +
               " GZ: " + String(gz2, 2);

    if (sendingEnabled) {
      publishSensorSnapshot();
    }
  }


if (M5.Mic.isEnabled() && !showLiveMessage) {

    auto data = &rec_data[rec_record_idx * record_length];

    if (M5.Mic.record(data, record_length, record_samplerate)) {
        if (++draw_record_idx >= record_number) {
            draw_record_idx = 0;
        }
        if (++rec_record_idx >= record_number) {
            rec_record_idx = 0;
        }
    }
}

  if (M5.BtnA.wasHold()) {
    auto cfg = M5.Mic.config();
    cfg.noise_filter_level = (cfg.noise_filter_level + 8) & 255;
    M5.Mic.config(cfg);

    M5.Display.clear();
    M5.Display.fillCircle(70, 15, 8, GREEN);
    M5.Display.drawString("NF:" + String(cfg.noise_filter_level), 120, 3);
  }

  else if (M5.BtnA.wasClicked()) {
    M5.Display.clear();
    while (M5.Mic.isRecording()) {
      delay(1);
    }

    M5.Mic.end();
    M5.Speaker.begin();

    M5.Display.fillTriangle(70 - 8, 15 - 8, 70 - 8, 15 + 8, 70 + 8, 15, 0x1c9f);
    M5.Display.drawString("PLAY", 120, 3);

    int start_pos = rec_record_idx * record_length;
  
    unsigned int audio_bytes = record_size * sizeof(int16_t);
    unsigned int send_bytes  = audio_bytes;

    uint8_t *byte_ptr = reinterpret_cast<uint8_t *>(rec_data);

    size_t total      = send_bytes;
    size_t num_chunks = (total + MIC_CHUNK_SIZE - 1) / MIC_CHUNK_SIZE;

    Serial.printf("Preparing to send %u bytes of audio in %u chunk(s)\n", send_bytes, static_cast<unsigned int>(num_chunks));

    bool all_chunks_ok = true;

    for (size_t chunk_index = 0; chunk_index < num_chunks; ++chunk_index) {
        size_t offset    = chunk_index * MIC_CHUNK_SIZE;
        size_t remaining = total - offset;
        size_t chunk_len = (remaining > MIC_CHUNK_SIZE) ? MIC_CHUNK_SIZE : remaining;

        bool ok_chunk = client.publish(mqtt_mic_bytes.c_str(), byte_ptr + offset, chunk_len, false);

        Serial.printf("Chunk %u/%u: offset=%u, len=%u -> %s\n",
                      static_cast<unsigned int>(chunk_index + 1),
                      static_cast<unsigned int>(num_chunks),
                      static_cast<unsigned int>(offset),
                      static_cast<unsigned int>(chunk_len),
                      ok_chunk ? "OK" : "FAIL");

        if (!ok_chunk) {
            all_chunks_ok = false;
        }
    }

    char meta_buf[64];
    snprintf(meta_buf, sizeof(meta_buf), "numbytes: %u", send_bytes);
    bool ok_meta = client.publish(
        mqtt_mic_meta.c_str(),
        meta_buf
    );

    Serial.printf("MQTT mic bytes (all chunks): %s (%u bytes in %u chunk(s))\n",
                  all_chunks_ok ? "ALL_OK" : "SOME_FAIL",
                  send_bytes,
                  static_cast<unsigned int>(num_chunks));

    Serial.printf("MQTT mic meta: %s (%s)\n",
                  ok_meta ? "OK" : "FAIL",
                  meta_buf);

    if (start_pos < (int)record_size) {
      M5.Speaker.playRaw(&rec_data[start_pos],
                         record_size - start_pos,
                         record_samplerate, false, 1, 0);
    }
    if (start_pos > 0) {
      M5.Speaker.playRaw(rec_data,
                         start_pos,
                         record_samplerate, false, 1, 0);
    }

    do {
      delay(1);
      M5.update();
    } while (M5.Speaker.isPlaying());

    M5.Speaker.end();
    M5.Mic.begin();

    M5.Display.clear();
    M5.Display.fillCircle(70, 15, 8, RED);
    M5.Display.drawString("REC", 120, 3);
  }

  static String lastTopic         = "";
  static String lastPayload       = "";
  static bool   lastSendingState  = false;
  static bool   firstGuiDraw      = true;

  if (showLiveMessage && currentTopic.length() > 0) {
    bool changed =
        firstGuiDraw ||
        (currentTopic    != lastTopic) ||
        (currentPayload  != lastPayload) ||
        (sendingEnabled  != lastSendingState);

    if (changed) {

      M5.Display.fillScreen(sendingEnabled ? GREEN : BLUE);

      String shortTopic = getShortTopic(currentTopic);

      String fullLine = currentTopic;
      if (fullLine.length() > 24) {

        fullLine = "…" + fullLine.substring(fullLine.length() - 24);
      }

      int screenH = M5.Display.height();
      int centerY = screenH / 2;
      int screenW = M5.Display.width();
      int centerX = screenW / 2;

      M5.Display.setTextDatum(top_left);
      M5.Display.setTextColor(WHITE);
      M5.Display.setFont(&fonts::FreeSansBold9pt7b);
      M5.Display.setTextSize(1);
      M5.Display.setCursor(2, 2);
      M5.Display.print(fullLine);

      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(WHITE);
      M5.Display.setFont(&fonts::FreeSansBold9pt7b);
      M5.Display.setTextSize(2);
      M5.Display.drawString(shortTopic, centerX, centerY - 15);

      M5.Display.setTextSize(2);
      M5.Display.drawString(currentPayload, centerX, centerY + 25);

      lastTopic        = currentTopic;
      lastPayload      = currentPayload;
      lastSendingState = sendingEnabled;
      firstGuiDraw     = false;
    }
  }else {
    firstGuiDraw = true;
  }
  }

/*---------------------------------------------------------
*  FUNCTION      : publishSensorSnapshot
*  DESCRIPTION   :
*    Publishes the current sensor readings to their respective MQTT topics.
*  PARAMETERS    : None
*  RETURNS       : None
*---------------------------------------------------------*/
void publishSensorSnapshot() {
  if (!client.connected()) return;

  int n = 0;

  n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", sht_temp);
  if (n > 0) client.publish(mqtt_sht_temperature.c_str(), mqtt_buffer);

  n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", sht_hum);
  if (n > 0) client.publish(mqtt_sht_humidity.c_str(), mqtt_buffer);

  n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", bmp_temp);
  if (n > 0) client.publish(mqtt_bmp_temperature.c_str(), mqtt_buffer);

  n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%.2f", bmp_pres);
  if (n > 0) client.publish(mqtt_bmp_pressure.c_str(), mqtt_buffer);

  n = snprintf(mqtt_buffer, sizeof(mqtt_buffer), "%d", battery_mv);
  if (n > 0) client.publish(mqtt_battery.c_str(), mqtt_buffer);

  if (imu_accel.length() > 0) {
    client.publish((mqtt_base + "imu/accel").c_str(), imu_accel.c_str());
  }
  if (imu_gyro.length() > 0) {
    client.publish((mqtt_base + "imu/gyro").c_str(), imu_gyro.c_str());
  }

  Serial.println("Published sensor values to individual MQTT topics");
}
