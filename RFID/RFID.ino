#include <esp_now.h>
#include <WiFi.h>
#include <MFRC522.h> //library responsible for communicating with the module RFID-RC522
#include <SPI.h> //library responsible for communicating of SPI bus

#define SS_PIN 21
#define RST_PIN 22
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16
#define greenPin 12
#define redPin 32

#define ptd portTICK_PERIOD_MS

//used in authentication
MFRC522::MIFARE_Key key;
//authentication return status code
MFRC522::StatusCode status;
// Defined pins to module RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

uint8_t broadcastAddress[] = {0x3c, 0x61, 0x05, 0x03, 0xb4, 0x4c}; // master MAC

TaskHandle_t notiTask = NULL;

typedef struct reg_message {
  int _val; // 0 = invalid, 1 = valid, 2 = button OTP verify successful, 3 = button OTP verify failed, 4 = RFID UUID recv, 5 = RFID write confirm
  char uuid[8]; // for RFID uuid, if _val is 2 then this will be "11111111" for yes and "00000000" for no.
} reg_message;
reg_message myData;

typedef struct internal_message {
  char _msg[8]; // 00000000 = open, 11111111 = fail if _type = 1 _msg is otp
  int _type; // 0 = NORM, 1 = SIGNUP, 2 = OTP FAIL, 3 = READ RFID, 4 = WRITE RFID
} internal_message;
internal_message msg;

int led = 2; // for indicate
int btn = 34; // for open and erase
int btn2 = 35; // for write
int state = 0; // 0 = NORM, 1 = READ, 2 = WRITE
int timeout = millis(); // timeout for all activity
char uuid[8]; // uuid of the RFID card
bool res; // state ctrl
byte buffer[SIZE_BUFFER] = {0}; // buffer for read data
byte writeBuffer[SIZE_BUFFER]; // buffer for write data
byte startState[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // RFID start state
char writeData[16];
bool spc = false;
bool notiAct = false;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { // execute on send
  if (status == ESP_NOW_SEND_SUCCESS) { // for indicate
    led_blink(5, 100);
  } else {
    led_blink(10, 50);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) { // execute on recv esp-now
  memcpy(&msg, incomingData, sizeof(msg));
  if (msg._type == 3) { // read RFID for signup
    state = 1;
  } else if (msg._type == 4) { // write RFID for signup
    state = 2;
  }
  RFID_CTRL(); // execute READ() or WRITE() func
  if (!res) {
    strcpy(myData.uuid, "0"); // fail
  } else {
    strcpy(myData.uuid, uuid); // success
  }

  digitalWrite(led, HIGH);
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // send data back to master esp32
  digitalWrite(led, LOW);

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

void READ() { // read key and uuid of the RFID card
  myData._val = 4;
  while (millis() - timeout <= 60000) {

    //prints the technical details of the card/tag
    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
    memcpy(&uuid, mfrc522.uid.uidByte, sizeof(uuid));

    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    //the block to operate
    byte block = 1;
    byte size = SIZE_BUFFER; //authenticates the block to operate
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Authentication failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      res = false;
      break;
    }
    else {
      //read data from block
      status = mfrc522.MIFARE_Read(block, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        res = false;
        break;
      }

      else {
        Serial.print(F("\nData from block ["));
        Serial.print(block); Serial.print(F("]: "));
        //prints read data
        bufferPrint();
        res = true;
        break;
      }
    }
  }
}

void WRITE(char cardKey[16]) { // write key and uuid of the RFID card
  myData._val = 5;
  memcpy(&writeBuffer, cardKey, 16); // default key
  while (millis() - timeout <= 60000) {

    //prints thecnical details from of the card/tag
    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
    memcpy(&uuid, mfrc522.uid.uidByte, sizeof(uuid));

    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    byte block; //the block to operate
    byte dataSize = 16; //size of data (bytes)

    block = 1; //the block to operate
    String str = "ku_kod_lnw$482"; //transforms the buffer data in String
    Serial.println(str);

    //authenticates the block to operate
    //Authenticate is a command to hability a secure communication
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                      block, &key, &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      res = false;
      break;
    }

    //Writes in the block
    status = mfrc522.MIFARE_Write(block, writeBuffer, MAX_SIZE_BLOCK);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      res = false;
      break;
    }
    else {
      Serial.println(F("MIFARE_Write() success: "));
      res = true;
      break;
    }

  }
}

void RFID_CTRL() { // signup ctrl func
  if (state == 1) {
    timeout = millis();
    READ();
    state = 0;
  } else if (state == 2) {
    timeout = millis();
    if (!spc) {
      strcpy(writeData, "ku_kod_lnw$482__");
    }
    WRITE(writeData);
    state = 0;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(btn, INPUT);
  pinMode(btn2, INPUT);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent); // set on send func cb
  esp_now_register_send_cb(OnDataSent); // set on recv func cb

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Init MFRC522
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init();

  xTaskCreatePinnedToCore(noti, "noti send", 1024, NULL, 1, &notiTask, 0);

  Serial.println("Approach your reader card...");
  Serial.println();
}

bool chk_RFID() { // checking RFID here
  timeout = millis(); // set timeout
  READ(); // get password and uuid
  if (res) { // read success
    char key[] = {'k', 'u', '_', 'k', 'o', 'd', '_', 'l', 'n', 'w', '$', '4', '8', '2', '_', '_'}; // default key
    int myStatus = 200; // status

    //prints read data
    bufferPrint(); // for debug
    for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) { // check the RFID's key
      Serial.printf("%c ----------- %c\n", buffer[i], key[i]);
      if ((buffer[i] != ' ') && (buffer[i] != key[i])) {
        myStatus = 401;
        break;
      }
    }

    if ( myStatus == 200 ) { // valid
      Serial.print("\n200 OK\n");
      return true;
    }
    else { // invalid
      Serial.print("\n401 Unauthorized\n");
      return false;
    }
  } else { // fail to read
    return res;
  }
}

void led_blink(int times, int _vTaskDelay) { // just led blinker func
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH);
    vTaskDelay(_vTaskDelay / ptd);
    digitalWrite(led, LOW);
    vTaskDelay(_vTaskDelay / ptd);
  }
}

void bufferPrint() { // just buffer print func
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    if (buffer[i] != ' ') {
      Serial.write(buffer[i]);
    }
  }
  Serial.println("");
}

void noti(void * param) {
  while (1) {
    if (notiAct) {
      strcpy(myData.uuid, "01011010");
      myData._val = 0; // unlock failed
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // send fail data to master esp32
      notiAct = false;
    }
    vTaskDelay(10 / ptd);
  }
}

void loop() {
  if (digitalRead(btn)) { // open by btn
    bool write_state = false;
    int write_timeout = millis();
    while (digitalRead(btn)) {
      write_state = false;
      if (millis() - write_timeout > 10000) {
        write_state = true;
        Serial.println("write sequence initiate");
        spc = true;
        notiAct = true;
        break;
      }
    }
    if (!write_state) {
      myData._val = 1; // open sig
      digitalWrite(led, HIGH);
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // send open data to master esp32
      digitalWrite(led, LOW);

      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
      vTaskDelay(5000 / ptd); // wait for door to lock
    } else  {
      while (1) {
        if ( ! mfrc522.PICC_IsNewCardPresent()) { // find card
          continue;
        }
        if ( ! mfrc522.PICC_ReadCardSerial()) { // select card
          continue;
        }
        state = 2;
        strcpy(writeData, "ku_kod_lnw$482__");
        RFID_CTRL();
        spc = false;
        notiAct = true;

        //instructs the PICC when in the ACTIVE state to go to a "STOP" state
        mfrc522.PICC_HaltA();
        // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
        mfrc522.PCD_StopCrypto1();
        vTaskDelay(500 / ptd);
        break;
      }
    }
  }
  if (digitalRead(btn2)) { // open by btn
    bool erase_state = false;
    int erase_timeout = millis();
    while (digitalRead(btn2)) {
      erase_state = false;
      if (millis() - erase_timeout > 10000) {
        erase_state = true;
        Serial.println("erase sequence initiate");
        spc = true;
        notiAct = true;
        break;
      }
    }
    if (erase_state) {
      while (1) {
        if ( ! mfrc522.PICC_IsNewCardPresent()) { // find card
          continue;
        }
        if ( ! mfrc522.PICC_ReadCardSerial()) { // select card
          continue;
        }
        state = 2;
        memcpy(writeData, startState, 16);
        RFID_CTRL();
        spc = false;
        notiAct = true;

        //instructs the PICC when in the ACTIVE state to go to a "STOP" state
        mfrc522.PICC_HaltA();
        // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
        mfrc522.PCD_StopCrypto1();
        vTaskDelay(500 / ptd);
        break;
      }
    }
  }
  if ( ! mfrc522.PICC_IsNewCardPresent()) { // find card
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) { // select card
    return;
  }
  if (chk_RFID() && !state) { // if in normal state and the RFID card is valid
    myData._val = 1; // open sig
    digitalWrite(led, HIGH);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // send open data to master esp32
    digitalWrite(led, LOW);

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    vTaskDelay(5000 / ptd); // wait for door to lock
    memcpy(buffer, startState, sizeof(buffer)); // reset buffer
  } else {
    strcpy(myData.uuid, "00000000");
    myData._val = 0; // unlock failed
    digitalWrite(led, HIGH);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // send fail data to master esp32
    digitalWrite(led, LOW);

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    vTaskDelay(3000 / ptd); // small vTaskDelay
  }
  //instructs the PICC when in the ACTIVE state to go to a "STOP" state
  mfrc522.PICC_HaltA();
  // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
  mfrc522.PCD_StopCrypto1();
}
