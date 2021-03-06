#include <esp_now.h>
#include <WiFi.h>
#include <MFRC522.h> //library responsible for communicating with the module RFID-RC522
#include <SPI.h> //library responsible for communicating of SPI bus
#include "mbedtls/aes.h"

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

uint8_t broadcastAddress[] = {0x24, 0x6f, 0x28, 0x50, 0xc0, 0xa0}; // master MAC

TaskHandle_t notiTask = NULL;

typedef struct reg_message {
  int _val; // 0 = invalid, 1 = valid, 2 = button OTP verify successful, 3 = button OTP verify failed, 4 = RFID UUID recv, 5 = RFID write confirm
  char uuid[9]; // for RFID uuid, if _val is 2 then this will be "11111111" for yes and "00000000" for no.
} reg_message;
reg_message myData;

typedef struct internal_message {
  char _msg[9]; // 00000000 = open, 11111111 = fail if _type = 1 _msg is otp
  int _type; // 0 = NORM, 1 = SIGNUP, 2 = OTP FAIL, 3 = READ RFID, 4 = WRITE RFID
} internal_message;
internal_message msg;

int led = 2; // for indicate
int btn = 34; // for open and erase
int btn2 = 35; // for write
int state = 0; // 0 = NORM, 1 = READ, 2 = WRITE
int timeout = millis(); // timeout for all activity
char uuid[8]; // uuid of the RFID card
char rev_uuid[9];
bool res; // state ctrl
byte buffer[SIZE_BUFFER] = {0}; // buffer for read data
byte writeBuffer[SIZE_BUFFER]; // buffer for write data
byte startState[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // RFID start state
unsigned char writeData[17];
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
  Serial.println(msg._type);
  if (msg._type == 1) {
    state = 1;
  }
}

void encrypt(char * plainText, char * key, unsigned char * outputBuffer) {

  mbedtls_aes_context aes;

  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)plainText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void decrypt(unsigned char * chipherText, char * key, unsigned char * outputBuffer) {

  mbedtls_aes_context aes;

  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_dec( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)chipherText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void reverse(char * string, int len) {
  char res[len + 1];
  for (int i = 0; i < len; i++) {
    res[i] = string[len - i - 1];
  }
  res[len] = '\0';
  strcpy(rev_uuid, res);
}


void READ() { // read key and uuid of the RFID card
  while (millis() - timeout <= 60000) {
    Serial.println("in read");
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
        for (int i = 0; i < 16; i++) {
          char tmp_hex[3];
          tmp_hex[0] = "0123456789ABCDEF"[buffer[i] / 16];
          tmp_hex[1] = "0123456789ABCDEF"[buffer[i] % 16];
          tmp_hex[2] = '\0';
          Serial.print(tmp_hex);
          if (i == 15) {
            Serial.println("");
          } else {
            Serial.print(" ");
          }
        }
        res = true;
        break;
      }
    }
  }
}

void WRITE(unsigned char cardKey[16]) { // write key and uuid of the RFID card
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
  timeout = millis();
  READ();

  if (!res) {
    strcpy(myData.uuid, "0"); // fail
  } else {
    //    memcpy(&myData.uuid, uuid, sizeof(uuid)); // success
    char res_[9];
    int index = 0;
    for (byte i = 0; i < sizeof(uuid); i++) {
      res_[index] = "0123456789ABCDEF"[uuid[i] / 16];
      res_[index + 1] = "0123456789ABCDEF"[uuid[i] % 16];
      index += 2;
    }
    res_[8] = '\0';
    strcpy(myData.uuid, res_);
  }

  Serial.println("Reading RFID UUID!");
  Serial.print("get >>> ");
  Serial.println(myData.uuid);

  myData._val = 4;

  digitalWrite(led, HIGH);
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // send data back to master esp32
  digitalWrite(led, LOW);

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  timeout = millis();
  if (!spc) {
    char k[20];
    strcpy(k, myData.uuid);
    reverse(myData.uuid, 8);
    strcat(k, rev_uuid);
    encrypt("ku_kod_lnw$482__", k, writeData);
  }
  Serial.println("writeData prepare");
  WRITE(writeData);

  if (!res) {
    strcpy(myData.uuid, "0"); // fail
  }

  Serial.println("Writing RFID UUID!");
  Serial.print("get >>> ");
  Serial.println(res);

  myData._val = 5;

  digitalWrite(led, HIGH);
  esp_err_t result2 = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // send data back to master esp32
  digitalWrite(led, LOW);

  if (result2 == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  state = 0;
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
  esp_now_register_recv_cb(OnDataRecv); // set on recv func cb

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

  if (!res) {
    memcpy(&myData.uuid, "0", 1); // fail
  } else {
    //    memcpy(&myData.uuid, uuid, sizeof(uuid)); // success
    char res_[9];
    int index = 0;
    for (byte i = 0; i < sizeof(uuid); i++) {
      res_[index] = "0123456789ABCDEF"[uuid[i] / 16];
      res_[index + 1] = "0123456789ABCDEF"[uuid[i] % 16];
      index += 2;
    }
    res_[8] = '\0';
    strcpy(myData.uuid, res_);
  }

  if (res) { // read success
    //    char key[] = {'k', 'u', '_', 'k', 'o', 'd', '_', 'l', 'n', 'w', '$', '4', '8', '2', '_', '_'}; // default key
    unsigned char key[17];
    int myStatus = 200; // status


    char k[20]; // decrypt key
    strcpy(k, myData.uuid);
    reverse(myData.uuid, 8);
    strcat(k, rev_uuid);
    encrypt("ku_kod_lnw$482__", k, key);

    //prints read data
    for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) { // check the RFID's key
      char read_hex[3];
      read_hex[0] = "0123456789ABCDEF"[buffer[i] / 16];
      read_hex[1] = "0123456789ABCDEF"[buffer[i] % 16];
      read_hex[2] = '\0';

      char key_hex[3];
      key_hex[0] = "0123456789ABCDEF"[key[i] / 16];
      key_hex[1] = "0123456789ABCDEF"[key[i] % 16];
      key_hex[2] = '\0';

      if ((buffer[i] != ' ') && (strcmp(read_hex, key_hex) != 0)) {
        myStatus = 401;
        break;
      }
      
      Serial.print(read_hex);
      Serial.print(" ----------- ");
      Serial.println(key_hex);
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
  if (!state) {
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
        strcpy(myData.uuid, "");

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
          spc = false;
          RFID_CTRL();
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
    if (chk_RFID()) { // if in normal state and the RFID card is valid
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
  } else {
    if ( ! mfrc522.PICC_IsNewCardPresent()) { // find card
      return;
    }
    if ( ! mfrc522.PICC_ReadCardSerial()) { // select card
      return;
    }
    RFID_CTRL(); // execute READ() or WRITE() func
    //instructs the PICC when in the ACTIVE state to go to a "STOP" state
    mfrc522.PICC_HaltA();
    // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
    mfrc522.PCD_StopCrypto1();
    int wait = millis();
    while (millis() - wait <= 3000);
  }
}
