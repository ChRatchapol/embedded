#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0x3c, 0x61, 0x05, 0x03, 0xb4, 0x4c}; // master MAC

typedef struct reg_message {
  int _val; // 0 = invalid, 1 = valid
} reg_message;
reg_message myData;

typedef struct OLED_BTN_message {
  char _msg[5]; // [0,0,0,0,0] is placeholder
  int _type; // 0 = NORM, 1 = SIGNUP
} OLED_BTN_message;
OLED_BTN_message msg;

int led = 2; // for indicate

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { // execute on send
  Serial.print("\r\nLast Packet Send Status:\t");
  if (status == ESP_NOW_SEND_SUCCESS) { // for indicate
    Serial.println("Delivery Success");
    led_blink(5, 100);
  } else {
    Serial.println("Delivery Fail");
    led_blink(10, 50);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&msg, incomingData, sizeof(msg));
  if (msg._type) {
    SIGNUP();
    led_blink(5, 100);
  } else {
    led_blink(2, 50);
  }
  Serial.println(msg._type);
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void SIGNUP() {
  Serial.println("SIGNUP");  
}

bool chk_btn() { // checking btn here
  return true; // if open return true else false
}

void led_blink(int times, int _delay) {
  for (int i=0;i<times;i++) {
    digitalWrite(led, HIGH);
    delay(_delay);
    digitalWrite(led, LOW);
    delay(_delay);
  }
}

void loop() {
  if (chk_btn()) {
    myData._val = 1;
  
    digitalWrite(led, HIGH);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    digitalWrite(led, LOW);
  
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  } else {}
  delay(5000); // for debug only
}
