#include <esp_now.h>
#include <WiFi.h>
#include <string>

#define ptd portTICK_PERIOD_MS

uint8_t broadcastAddress[] = {0x24, 0x6f, 0x28, 0x50, 0xc0, 0xa0}; // master MAC // 24:6F:28:50:C0:A0

typedef struct reg_message {
  int _val; // 0 = invalid, 1 = valid, 2 = button OTP verify successful, 3 = button OTP verify failed, 4 = RFID UUID recv, 5 = RFID write confirm
  char uuid[8]; // for RFID uuid, if _val is 2, 4, 5 then this will be "11111111..." for yes and "00000000..." for no.
} reg_message;
reg_message myData;

typedef struct internal_message {
  char _msg[9]; // 00000000 = open, 11111111 = fail if _type = 1 _msg is otp
  int _type; // 0 = NORM, 1 = SIGNUP, 2 = OTP FAIL, 3 = READ RFID, 4 = WRITE RFID, 5 = RFID FAIL
} internal_message;
internal_message msg;

int led = 2; // for indicate
int red = 34 , yellow = 35 , blue = 32 , green = 33; // for 4 button
int timeout = millis();
bool timeout_state = false;

TaskHandle_t chkBtnTask = NULL;
TaskHandle_t waitRFIDTask = NULL;

int stateRed = 0;
int stateBlu = 0;
int stateYlw = 0;
int stateGrn = 0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { // execute on send
  if (status == ESP_NOW_SEND_SUCCESS) { // for indicate
    led_blink(5, 100);
  } else {
    led_blink(10, 50);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&msg, incomingData, sizeof(msg));
  if (msg._type == 1) {
    xTaskCreatePinnedToCore(chk_btn, "chk btn", 1024, NULL, 1, &chkBtnTask, 0);
    led_blink(5, 100);
  } else {
    led_blink(2, 50);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(red,    INPUT_PULLUP);
  pinMode(yellow, INPUT_PULLUP);
  pinMode(blue,   INPUT_PULLUP);
  pinMode(green,  INPUT_PULLUP);

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

void waitRFID(void * param) {
  timeout = millis();
  int count;
  while (millis() - timeout <= 60000) {
    if (!digitalRead(green)) {
      stateGrn = 0;
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateGrn = 1;
          break;
        } else {
          if (digitalRead(green)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
      if (stateGrn) {
        myData._val = 2;
        strcpy(myData.uuid, "1111111");
        break;
      }
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateGrn = 0;
          break;
        } else {
          if (!digitalRead(green)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
    } else if (!digitalRead(red)) {
      stateRed = 0;
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateRed = 1;
          break;
        } else {
          if (digitalRead(red)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
      if (stateRed) {
        myData._val = 2;
        strcpy(myData.uuid, "0000000");
        break;
      }
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateRed = 0;
          break;
        } else {
          if (!digitalRead(red)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
    }
    vTaskDelay(10 / ptd);
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
  vTaskDelete(NULL);
}

void chk_btn(void * param) { // checking btn here
  std::string otp = "";
  std::string cmp = "";
  for (int index = 0; index < 8; index++) {
    cmp = cmp + msg._msg[index];
  }
  int i = 0;
  timeout = millis();
  while (millis() - timeout <= 60000) {
    if (i == 8) {
      break;
    }
    int count = 200;
    if (!digitalRead(red)) {
      stateRed = 0;
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateRed = 1;
          break;
        } else {
          if (digitalRead(red)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
      if (stateRed) {
        otp = otp + 'R';
        Serial.println(otp.c_str());
        i++;
      }
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateRed = 0;
          break;
        } else {
          if (!digitalRead(red)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
    } else if (!digitalRead(yellow)) {
      stateYlw = 0;
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateYlw = 1;
          break;
        } else {
          if (digitalRead(yellow)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
      if (stateYlw) {
        otp = otp + 'Y';
        Serial.println(otp.c_str());
        i++;
      }
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateYlw = 0;
          break;
        } else {
          if (!digitalRead(yellow)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
    } else if (!digitalRead(blue)) {
      stateBlu = 0;
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateBlu = 1;
          break;
        } else {
          if (digitalRead(blue)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
      if (stateBlu) {
        otp = otp + 'B';
        Serial.println(otp.c_str());
        i++;
      }
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateBlu = 0;
          break;
        } else {
          if (!digitalRead(blue)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
    } else if (!digitalRead(green)) {
      stateGrn = 0;
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateGrn = 1;
          break;
        } else {
          if (digitalRead(green)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
      if (stateGrn) {
        otp = otp + 'G';
        Serial.println(otp.c_str());
        i++;
      }
      count = 70;
      while (1) {
        count--;
        if (count <= 0) {
          stateBlu = 0;
          break;
        } else {
          if (!digitalRead(green)) {
            count++;
            if (count >= 140) {
              break;
            }
          }
          delay(1);
        }
      }
    }
    vTaskDelay(10 / ptd);
  }
  if (cmp.compare(otp) == 0) {
    myData._val = 2;
    Serial.println("verify!");
    xTaskCreatePinnedToCore(waitRFID, "wait RFID", 1024, NULL, 1, &waitRFIDTask, 0);
  } else {
    Serial.println("invalid otp!");
    strcpy(myData.uuid, "00000000");
    myData._val = 3;

    digitalWrite(led, HIGH);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    digitalWrite(led, LOW);

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
  vTaskDelete(NULL);
}


void led_blink(int times, int _vTaskDelay) {
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH);
    vTaskDelay(_vTaskDelay / ptd);
    digitalWrite(led, LOW);
    vTaskDelay(_vTaskDelay / ptd);
  }
}

void loop() {
}
