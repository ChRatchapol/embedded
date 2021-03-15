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

//used in authentication
MFRC522::MIFARE_Key key;
//authentication return status code
MFRC522::StatusCode status;
// Defined pins to module RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

uint8_t broadcastAddress[] = {0x3c, 0x61, 0x05, 0x03, 0xb4, 0x4c}; // master MAC

typedef struct reg_message {
  int _val; // 0 = invalid, 1 = valid, 2 = button OTP verify successful, 3 = button OTP verify failed, 4 = RFID UUID recv, 5 = RFID write confirm
  char uuid[16]; // for RFID uuid, if _val is 2 then this will be "11111111" for yes and "00000000" for no.
} reg_message;
reg_message myData;

typedef struct internal_message {
  char _msg[8]; // 00000000 = open, 11111111 = fail if _type = 1 _msg is otp
  int _type; // 0 = NORM, 1 = SIGNUP, 2 = OTP FAIL, 3 = READ RFID, 4 = WRITE RFID
} internal_message;
internal_message msg;

int led = 2; // for indicate
int state = 0; // 0 = NORM, 1 = READ, 2 = WRITE
int timeout = millis();
char uuid[16];
bool res;
byte buffer[SIZE_BUFFER] = {0}; //buffer for read data
byte startState[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) { // execute on send
  if (status == ESP_NOW_SEND_SUCCESS) { // for indicate
    led_blink(5, 100);
  } else {
    led_blink(10, 50);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) { // execute on recv esp-now
  memcpy(&msg, incomingData, sizeof(msg));
  if (msg._type == 3) {
    state = 1;
  } else if (msg._type == 4) {
    state = 2;
  }
  RFID_CTRL();
  if (!res) {
    strcpy(myData.uuid, "0000000000000000");
  } else {
    strcpy(myData.uuid, "1111111111111111");
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
}

void READ() {
  myData._val = 4;
  while (millis() - timeout <= 60000) {

    //prints the technical details of the card/tag
    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));

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

void WRITE() {
  bool success = false;
  myData._val = 5;
  while (millis() - timeout <= 60000) {
    // WRITE RFID CODE
    success = true;
  }
  if (!success) {
    res = false;
  }
}

void RFID_CTRL() {
  if (state == 1) {
    timeout = millis();
    READ();
    state = 0;
  } else if (state == 2) {
    timeout = millis();
    WRITE();
    state = 0;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);

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
  Serial.println("Approach your reader card...");
  Serial.println();
}

bool chk_RFID() { // checking RFID here
  timeout = millis();
  READ();
  if (res) {
    char key[] = {'k', 'u', '_', 'k', 'o', 'd', '_', 'l', 'n', 'w', '$', '4', '8', '2'};
    int myStatus = 200;

    //prints read data
    bufferPrint();
    for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
      if ((buffer[i] != ' ') && (buffer[i] != key[i])) {
        myStatus = 401;
        break;
      }
    }

    if ( myStatus == 200 ) {
      Serial.print("\n200 OK");
      return true;
    }
    else {
      Serial.print("\n401 Unauthorized");
      return false;
    }
  } else {
    return res;
  }
}

void led_blink(int times, int _delay) {
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH);
    delay(_delay);
    digitalWrite(led, LOW);
    delay(_delay);
  }
}

void bufferPrint() {
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++) {
    if (buffer[i] != ' ') {
      Serial.write(buffer[i]);
    }
  }
  Serial.println("");
}

void loop() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select a card
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.printf("in loop <<< ");
  bufferPrint();
  Serial.println("pass card chk");
  if (chk_RFID() && !state) {
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
    delay(5000); // wait for door to lock
    memcpy(buffer, startState, sizeof(buffer));
  } else {
    myData._val = 0;
    digitalWrite(led, HIGH);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    digitalWrite(led, LOW);

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    delay(3000); // small delay
  }
  //instructs the PICC when in the ACTIVE state to go to a "STOP" state
  mfrc522.PICC_HaltA();
  // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
  mfrc522.PCD_StopCrypto1();
}
