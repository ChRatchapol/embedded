#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Wire.h>
#include <WireSlave.h>

#define ptd portTICK_PERIOD_MS

#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_SLAVE_ADDR 0x04

#define led 2 // for indicate
#define sig 23 // for relay

TaskHandle_t i2cTask = NULL;

const int _size = 2 * JSON_OBJECT_SIZE(5);

char str[100];
char recvStr[100];
char _UUID[8];

bool failed = false;
int state = 0; // 0 = idle, 1 = waiting for OTP verification, 2 = finishing signup sequence
int timeout = millis(); // unlock timeout
bool unlock_state = false;
int unlockFrom = 0; // 0 = RFID, 1 = LINE

StaticJsonDocument<_size> JSONRecv;
StaticJsonDocument<_size> JSONSend;

uint8_t broadcastAddressOLED[] = {0xa4, 0xcf, 0x12, 0x8f, 0xcc, 0x84}; // OLED_BUZZER MAC
uint8_t broadcastAddressBTN[]  = {0x30, 0xae, 0xa4, 0x12, 0x64, 0xe0}; // BUTTON MAC
uint8_t broadcastAddressRFID[] = {0xa4, 0xcf, 0x12, 0x8f, 0x88, 0x44}; // RFID MAC

typedef struct reg_message {
  int _val; // 0 = invalid, 1 = valid, 2 = button OTP verify successful, 3 = button OTP verify failed, 4 = RFID UUID recv, 5 = RFID write confirm
  char uuid[16]; // for RFID uuid, if _val is 2, 4, 5 then this will be "11111111..." for yes and "00000000..." for no.
} reg_message;
reg_message myData;

typedef struct internal_message {
  char _msg[8]; // 00000000 = open, 11111111 = fail if _type = 1 _msg is otp
  int _type; // 0 = NORM, 1 = SIGNUP, 2 = OTP FAIL, 3 = READ RFID, 4 = WRITE RFID, 5 = RFID FAIL
} internal_message;
internal_message msg;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { // execute on send esp-now
  if (status == ESP_NOW_SEND_SUCCESS) { // for indicate
    led_blink(5, 100);
  } else {
    led_blink(10, 50);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) { // execute on recv esp-now
  memcpy(&myData, incomingData, sizeof(myData));
  if (myData._val == 1) { // open sig
    strcpy(msg._msg, "00000000"); // tell OLED to show open sequence on screen
    msg._type = 0;
    OPEN(); // open
    led_blink(5, 100);
  } else if (myData._val == 0) { // fail sig
    strcpy(msg._msg, "11111111"); // tell OLED to show fail sequence on screen
    msg._type = 0;
    led_blink(2, 50);
  } else if (myData._val == 2) { // button OTP verify successful
    if (strcmp(myData.uuid, "1111111111111111")) {
      strcpy(msg._msg, "11111111"); // just placeholder
      msg._type = 3; // RFID Read

      digitalWrite(led, HIGH);
      esp_err_t result = esp_now_send(broadcastAddressRFID, (uint8_t *) &msg, sizeof(msg)); // tell RFID to read uuid and send back
      digitalWrite(led, LOW);

      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
    }
  } else if (myData._val == 3) { // button otp verify failed
    state = 0;
    failed = true;
    strcpy(msg._msg, "11111111"); // just placeholder
    msg._type = 2; // tell OLED to show OTP verification fail
  } else if (myData._val == 4) { // RFID UUID recv
    if (strcmp(myData.uuid, "0000000000000000") == 0) {
      state = 0;
      failed = true;
      msg._type = 5;
    } else {
      strcpy(_UUID, myData.uuid);
      state = 2; // send data to backend
      strcpy(msg._msg, "11111111"); // just placeholder
      msg._type = 0; // normal type
    }
  } else if (myData._val == 5) { // RFID Write finish
    if (strcmp(myData.uuid, "0000000000000000") == 0) {
      failed = true;
      msg._type = 5;
    } else {
      strcpy(msg._msg, "11111111"); // just placeholder
      msg._type = 0; // normal type
    }
    state = 0;
  }
  logging(); // send data to backend through i2c

  digitalWrite(led, HIGH);
  esp_err_t result = esp_now_send(broadcastAddressOLED, (uint8_t *) &msg, sizeof(msg)); // send
  digitalWrite(led, LOW);

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

void receiveEvent(int howMany) { // execute on recv i2c packet
  int i = 0;
  while (1 < WireSlave.available()) {
    char c = WireSlave.read();
    recvStr[i] = c;
    i++;
  }

  char x = WireSlave.read();
  recvStr[i] = x;

  DeserializationError err = deserializeJson(JSONRecv, recvStr);
  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
  } else {
    int _type_ = JSONRecv["type"];
    if ((_type_ == 1) && (state == 0)) { // start signup sequence
      strcpy(msg._msg, JSONRecv["OTP"]);
      msg._type = 1;

      digitalWrite(led, HIGH);
      esp_err_t result = esp_now_send(broadcastAddressOLED, (uint8_t *) &msg, sizeof(msg)); // send OTP to OLED
      digitalWrite(led, LOW);

      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }

      digitalWrite(led, HIGH);
      esp_err_t result2 = esp_now_send(broadcastAddressBTN, (uint8_t *) &msg, sizeof(msg)); // send OTP to button
      digitalWrite(led, LOW);

      if (result2 == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
      state = 1; // wait for OTP verification

    } else if ((_type_ == 3) && (state == 0)) { // unlock from LINE
      unlockFrom = 1;
      logging();
      OPEN(); // open

      strcpy(msg._msg, "11111111");
      msg._type = 0;

      digitalWrite(led, HIGH);
      esp_err_t result = esp_now_send(broadcastAddressOLED, (uint8_t *) &msg, sizeof(msg)); // tell OLED to show open sequence on screen
      digitalWrite(led, LOW);

      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
    } else if ((_type_ == 4) && (state == 0)) { // start writing RFID sequence
      strcpy(msg._msg, "11111111"); // just placeholder
      msg._type = 1; // tell RFID to start writing sequence

      digitalWrite(led, HIGH);
      esp_err_t result = esp_now_send(broadcastAddressRFID, (uint8_t *) &msg, sizeof(msg)); // send sig to RFID to start writing sequence
      digitalWrite(led, LOW);

      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }

      strcpy(msg._msg, "11111111"); // just placeholder
      msg._type = 4; // tell RFID to start writing sequence

      digitalWrite(led, HIGH);
      esp_err_t result2 = esp_now_send(broadcastAddressOLED, (uint8_t *) &msg, sizeof(msg)); // tell OLED to show RFID Write message sequence
      digitalWrite(led, LOW);

      if (result2 == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
    } else {
      failed = true; // Well, It's a fail state.
    }
  }
}

void requestEvent() { // execute on rqst i2c packet
  WireSlave.print(str);
  for (int i = 0; i < (70 - strlen(str)); i++) { // padding
    WireSlave.write(0x00);
  }
  if (unlockFrom == 1) {
    unlockFrom = 0;
  }
}

void setup() {
  Serial.begin(115200); // start serial
  pinMode(led, OUTPUT);
  pinMode(sig, OUTPUT);

  WiFi.mode(WIFI_STA); // for esp-now

  if (esp_now_init() != ESP_OK) { // init esp-now
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent); // set on send func cb
  esp_now_register_recv_cb(OnDataRecv); // set on recv func cb

  esp_now_peer_info_t peerInfo; // create peer info for esp-now
  peerInfo.channel = 0; // choose esp-now channel
  peerInfo.encrypt = false; // not use encrypt for esp-now

  memcpy(peerInfo.peer_addr, broadcastAddressOLED, 6); // registed OLED for esp-now
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddressBTN, 6); // registed BTN for esp-now
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddressRFID, 6); // registed RFID for esp-now
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  bool res = WireSlave.begin(SDA_PIN, SCL_PIN, I2C_SLAVE_ADDR); // start i2c connection with raspi (backend)
  if (!res) {
    Serial.println("I2C slave init failed");
    while (1) vTaskDelay(100 / ptd);
  }

  WireSlave.onReceive(receiveEvent); // on recv func cb for i2c
  WireSlave.onRequest(requestEvent); // on rqst func cb for i2c

  Serial.printf("Slave joined I2C bus with addr #%d\n", I2C_SLAVE_ADDR);

  xTaskCreatePinnedToCore(i2c_sending, "i2c send", 1024, NULL, 1, &i2cTask, 0); // seperate core for i2c sending
}

void led_blink(int times, int _delay) { // to blink led
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH);
    vTaskDelay(_delay / ptd);
    digitalWrite(led, LOW);
    vTaskDelay(_delay / ptd);
  }
}

void logging() { // func to send data through i2c
  JSONSend.clear(); // clear JSON
  if (failed) { // send fail JSON to backend with 255 code
    JSONSend["type"] = 255;
    failed = false;
  } else if (state == 2) { // send signup data to backend (only if in signup finishing state)
    JSONSend["type"] = 1;
    JSONSend["uuid"] = _UUID;
    JSONSend["OTP"] = msg._msg;
  } else {
    if ((myData._val) && (state == 0)) { // send normal logging when open the door
      JSONSend["type"] = 2;
      JSONSend["uuid"] = _UUID;
      if (unlockFrom == 0) {
        JSONSend["from"] = "RFID";
      } else if (unlockFrom == 1) {
        JSONSend["from"] = "LINE";
      }
    } else if (state == 0) { // send idle data
      JSONSend["type"] = 0;
    }
  }
  serializeJson(JSONSend, str);
}

void i2c_sending(void* param) { // always polling request from i2c master
  while (1) {
    WireSlave.update();
    vTaskDelay(1 / ptd);
  }
}

void OPEN() { // func that unlock the lock
  if (state == 0 && !unlock_state) { // only unlock if in idle state (not in signup sequence)
    digitalWrite(sig, LOW);
    timeout = millis();
    unlock_state = true;
  }
}

void loop() { // control timeout
  if (!unlock_state) { // if the door is not unlocked, reset timeout
    timeout = millis();
  } else if (unlock_state && (millis() - timeout >= 5000)) { // else if the door is unlocked and 5s pass, lock the door
    digitalWrite(sig, HIGH);
    unlock_state = false;
  }
}
