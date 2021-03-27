#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <SPI.h>

// BUZZER NOTE IMPLIMENT
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

#define BUZZER_PIN 13

bool blink_ = true;
bool fastBlink_ = true;
int time_ = millis();
int fastTime_ = millis();
int timeout = millis();
bool timeout_state = false;
String oled_msg;
bool signupState = false;

int _tempo = 25;

int _melody[] = {
  NOTE_C5,  75, REST, 300,
  NOTE_C5,  75, REST, 300,
  NOTE_C5,  75, REST, 300,
  NOTE_C5,  25, REST, 100,
  NOTE_GS4, 25, REST, 100,
  NOTE_AS4, 25, REST, 100,
  NOTE_C5,  75, REST, 200,
  NOTE_AS4, 75, REST, 150,
  NOTE_C5,  8, REST, 32

};

int fail_melody[] = {
  //  NOTE_FS4, 25, NOTE_F4, 12, REST, 32
  NOTE_C5,  40, REST, 300,
  NOTE_B4,  40, REST, 300,
  NOTE_AS4, 40, REST, 300,
  NOTE_A4,  75, NOTE_AS4, 75,
  NOTE_A4,  75, NOTE_AS4, 75,
  NOTE_A4,  20, REST, 100
};

int noti_melody[] = {
  NOTE_FS4, 75, REST, 300,
  NOTE_GS4, 75, REST, 300,
  NOTE_A4,  75, REST, 300
};

int _notes = sizeof(_melody) / sizeof(_melody[0]) / 2;
int fail_notes = sizeof(fail_melody) / sizeof(fail_melody[0]) / 2;
int noti_notes = sizeof(noti_melody) / sizeof(noti_melody[0]) / 2;

int _wholenotes = (60000 * 4) / _tempo;

int divider = 0, noteDuration = 0;

//END OF BUZZER NOTE


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(128, 64, &Wire, -1);

int state = 0;

typedef struct OLED_BTN_message {
  char _msg[9];
  int _type; // 0 = NORM, 1 = SIGNUP
} OLED_BTN_message;
OLED_BTN_message msg;

#define led 2

void tone(int freq, int duration) {
  ledcWriteTone(0, freq);
  delay(duration);
  ledcWriteTone(0, REST);
}

void OLED() {
  if (strcmp(msg._msg, "00000000") == 0) {
    Serial.println("Open");
    oled_msg = " Unlocked ";
    state = 1;
  } else if (strcmp(msg._msg, "11111111") == 0) {
    Serial.println("Cannot open");
    oled_msg = "  Unlock    Failed  ";
    state = 0;
  } else if (strcmp(msg._msg, "01011010") == 0) {
    oled_msg = "  SIGNUP     FIN.   ";
    state = 1;
  }
}

void SUCCESS_SONG() {
  for (int thisNote = 0; thisNote < _notes * 2; thisNote = thisNote + 2) {
    divider = _melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (_wholenotes) / divider;
    } else if (divider < 0) {
      // dotted _notes are represented with negative durations!!
      noteDuration = (_wholenotes) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    tone(_melody[thisNote], noteDuration);
  }
}

void FAIL_SONG() {
  for (int thisNote = 0; thisNote < fail_notes * 2; thisNote = thisNote + 2) {
    divider = fail_melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (_wholenotes) / divider;
    } else if (divider < 0) {
      // dotted _notes are represented with negative durations!!
      noteDuration = (_wholenotes) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    tone(fail_melody[thisNote], noteDuration);
  }
}

void NOTI_SONG() {
  for (int thisNote = 0; thisNote < noti_notes * 2; thisNote = thisNote + 2) {
    divider = noti_melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (_wholenotes) / divider;
    } else if (divider < 0) {
      // dotted _notes are represented with negative durations!!
      noteDuration = (_wholenotes) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    tone(noti_melody[thisNote], noteDuration);
  }
}

void BUZZER() {
  Serial.println("BUZZER");
  if (strcmp(msg._msg, "00000000") == 0) {
    SUCCESS_SONG();
  } else if (strcmp(msg._msg, "11111111") == 0) {
    FAIL_SONG();
  } else if (strcmp(msg._msg, "01011010") == 0) {
    NOTI_SONG();
  }
}

void SIGNUP() {
  Serial.println(msg._msg);
  char tmp[11] = {' ', ' ', 'O', 'T', 'P', ' ', 'i', 's', ' ', ' ', ' '};
  String _msg_;
  for (int index = 0 ; index < 11; index++) {
    _msg_ = _msg_ + tmp[index];
  }
  for (int index = 0 ; index < 8; index++) {
    _msg_ = _msg_ + msg._msg[index];
  }
  _msg_ = _msg_ + ' ';
  oled_msg = _msg_;
  state = 1;
  NOTI_SONG();
}

void OTP_FAIL() {
  Serial.println("OTP Fail");
  oled_msg = " OTP FAIL ";
  state = 0;
  FAIL_SONG();
}

void RFID() {
  Serial.println("RFID");
  oled_msg = "Place your   RFID   ";
  state = 1;
  NOTI_SONG();
}

void RFID_FAIL() {
  Serial.println("RFID Fail");
  oled_msg = "   RFID      FAIL   ";
  state = 0;
  FAIL_SONG();
}

void led_blink(int times, int _delay) {
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH);
    delay(_delay);
    digitalWrite(led, LOW);
    delay(_delay);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&msg, incomingData, sizeof(msg));
  Serial.println(msg._type);
  if (msg._type == 1) {
    signupState = true;
    SIGNUP();
  } else if (msg._type == 0) {
    signupState = false;
    oled_msg = "";
    timeout_state =  false;
    OLED();
    BUZZER();
  } else if (msg._type == 2) {
    signupState = false;
    oled_msg = "";
    timeout_state =  false;
    OTP_FAIL();
  } else if (msg._type == 3) {
    oled_msg = "";
    timeout_state =  false;
    RFID();
  } else if (msg._type == 4 && !signupState) {
    RFID_FAIL();
  }
  timeout = millis();
  timeout_state = true;
  led_blink(5, 100);
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void loop() {
  if (!timeout_state) {
    timeout = millis();
  }
  if (millis() - time_ >= 1000) {
    blink_ = !blink_;
    time_ = millis();
  }
  if (millis() - fastTime_ >= 500) {
    fastBlink_ = !fastBlink_;
    fastTime_ = millis();
  }
  if (!signupState) {
    if (millis() - timeout >= 3500) {
      oled_msg = "";
      timeout_state =  false;
    }
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 0);
  display.println(" SoupDoor ");
  //==============================
  display.setCursor(0, 20);
  display.setTextSize(2);
  if ((fastBlink_ && !state) || state) {
    display.setTextColor(WHITE, BLACK);
  } else if (!state) {
    display.setTextColor(BLACK, WHITE);
  }
  display.println(oled_msg);
  //==============================
  if (blink_) {
    display.fillCircle(3, 64 - 3 - 1, 3, WHITE);
  } else {
    display.drawCircle(3, 64 - 3 - 1, 2, WHITE);
  }
  display.display();
  //  Serial.print("Here <<<< ");
  //  Serial.println(oled_msg);
}
