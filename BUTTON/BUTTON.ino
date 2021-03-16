#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0x3c, 0x61, 0x05, 0x03, 0xb4, 0x4c}; // master MAC

typedef struct reg_message {
  int _val; // 0 = invalid, 1 = valid, 2 = button OTP verify successful, 3 = button OTP verify failed, 4 = RFID UUID recv, 5 = RFID write confirm
  char uuid[8]; // for RFID uuid, if _val is 2, 4, 5 then this will be "11111111..." for yes and "00000000..." for no.
} reg_message;
reg_message myData;

typedef struct internal_message {
  char _msg[8]; // 00000000 = open, 11111111 = fail if _type = 1 _msg is otp
  int _type; // 0 = NORM, 1 = SIGNUP, 2 = OTP FAIL, 3 = READ RFID, 4 = WRITE RFID, 5 = RFID FAIL
} internal_message;
internal_message msg;

int led = 2; // for indicate
int red = 34 , yellow = 35 , blue = 32 , green = 33; // for 4 button
int timeout = millis();
bool timeout_state = false;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { // execute on send
  if (status == ESP_NOW_SEND_SUCCESS) { // for indicate
    //    Serial.println("Delivery Success");
    led_blink(5, 100);
  } else {
    //    Serial.println("Delivery Fail");
    led_blink(10, 50);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&msg, incomingData, sizeof(msg));
  if (msg._type == 1) {
    chk_btn(); // check OTP
    led_blink(5, 100);
  } else {
    led_blink(2, 50);
  }
  Serial.println(msg._type);
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(red, INPUT_PULLUP);
  pinMode(yellow, INPUT_PULLUP);
  pinMode(blue, INPUT_PULLUP);
  pinMode(green, INPUT_PULLUP);

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

void waitRFID() {
  timeout = millis();
  while (millis() - timeout <= 60000) {
    if (!digitalRead(green)) {
      myData._val = 2;
      strcpy(myData.uuid, "1111111");
      break;
    } else if (!digitalRead(red)) {
      myData._val = 2;
      strcpy(myData.uuid, "0000000");
      break;
    }
  }
}

void chk_btn() { // checking btn here
  char otp[8];
  int i = 0;
  timeout = millis();
  while (millis() - timeout <= 60000) {
    if (i == 8) {
      break;
    }
    if (!digitalRead(red)) {
      otp[i] = 'r';
      Serial.println(otp);
      i++;
    }
    else if (!digitalRead(yellow)) {
      otp[i] = 'y';
      Serial.println(otp);
      i++;
    }
    else if (!digitalRead(blue)) {
      otp[i] = 'b';
      Serial.println(otp);
      i++;
    }
    else if (!digitalRead(green)) {
      otp[i] = 'g';
      Serial.println(otp);
      i++;
    }
  }
  if (strcmp(msg._msg, otp) == 0) {
    strcpy(myData.uuid, "11110000");
    myData._val = 2;
  }
  else {
    strcpy(myData.uuid, "00001111");
    myData._val = 3;
  }

  digitalWrite(led, HIGH);
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  digitalWrite(led, LOW);

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  waitRFID();
}


void led_blink(int times, int _delay) {
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH);
    delay(_delay);
    digitalWrite(led, LOW);
    delay(_delay);
  }
}

void loop() {
}
