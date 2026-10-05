#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "USB.h"
#include "USBHIDMouse.h"

USBHIDMouse Mouse;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Preferences preferences;
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))
void drawLoadingScreen(unsigned long durationMs);
void printCenteredText(String text, uint8_t size);
uint8_t mouseSensitivity = 5;
uint8_t scrollSensitivity = 1;
bool settingsDirty = false;
uint16_t flappyHighScore = 0;

void loadSettingsFromNVS() {
  preferences.begin("game_cfg", true);
  mouseSensitivity = preferences.getUChar("mouseSens", 5);
  scrollSensitivity = preferences.getUChar("scrollSens", 1);
  flappyHighScore = preferences.getUShort("flappyHigh", 0);
  preferences.end();
}

void saveSettingsToNVS() {
  preferences.begin("game_cfg", false);
  preferences.putUChar("mouseSens", mouseSensitivity);
  preferences.putUChar("scrollSens", scrollSensitivity);
  preferences.putUShort("flappyHigh", flappyHighScore);
  preferences.end();
  settingsDirty = false;
}

const uint8_t ru = 12;
const uint8_t rd = 10;
const uint8_t rl = 11;
const uint8_t rr = 13;
const uint8_t rs = 9;

const uint8_t lu = 5;
const uint8_t ld = 7;
const uint8_t ll = 4;
const uint8_t lr = 6;
const uint8_t ls = 15;
unsigned long cnt = 0;
struct FlappyPipe {
  int x;
  int gapY;
  int gapHeight;
};

bool flappyStarted = false;
bool flappyGameOver = false;
bool flappyPaused = false;
int flappyBirdY = 24;
int flappyBirdVy = 0;
int flappyScore = 0;
unsigned long flappyLastFrame = 0;
unsigned long flappyPauseUntil = 0;
FlappyPipe flappyPipes[3];

void startFlappyGame() {
  flappyStarted = true;
  flappyGameOver = false;
  flappyBirdY = 24;
  flappyBirdVy = 0;
  flappyScore = 0;
  flappyLastFrame = millis();
  flappyPaused = true;
  flappyPauseUntil = millis() + 1400;
  for (int i = 0; i < 3; i++) {
    flappyPipes[i].x = 90 + i * 58;
    flappyPipes[i].gapY = random(12, 40);
    flappyPipes[i].gapHeight = 28;
  }
}

bool flappyCollides(int pipeX, int pipeGapY, int pipeGapHeight) {
  int birdLeft = 20;
  int birdRight = 28;
  int birdTop = flappyBirdY;
  int birdBottom = flappyBirdY + 8;
  if (birdRight >= pipeX && birdLeft <= pipeX + 6) {
    if (birdTop < pipeGapY || birdBottom > pipeGapY + pipeGapHeight) {
      return true;
    }
  }
  return birdBottom >= 54;
}

void drawFlappyGame(bool showPausePrompt = false) {
  display.clearDisplay();
  display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
  for (int i = 0; i < 3; i++) {
    int x = flappyPipes[i].x;
    int gapY = flappyPipes[i].gapY;
    int gapH = flappyPipes[i].gapHeight;
    display.fillRect(x, 0, 6, gapY, SSD1306_WHITE);
    display.fillRect(x, gapY + gapH, 6, 64 - (gapY + gapH), SSD1306_WHITE);
  }
  display.fillRect(20, flappyBirdY, 8, 8, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 56);
  display.print("Score:");
  display.print(flappyScore);
  display.setCursor(72, 56);
  display.print("Hi:");
  display.print(flappyHighScore);
  if (showPausePrompt) {
    display.setCursor(24, 18);
    display.print("Ready?");
  }
  display.display();
}

void drawFlappyGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 10);
  display.print("OVER");
  display.setTextSize(1);
  display.setCursor(10, 34);
  display.print("Score:");
  display.print(flappyScore);
  display.setCursor(10, 46);
  display.print("RS=Retry  LS=Menu");
  display.display();
}


const String modes[6]{ "menu", "DinoGame", "Tetris", "FlappyBird", "Mouse", "Settings" };
int mode = 0;
const String viewingModes[5]{ "DinoGame", "Tetris", "FlappyBird", "Mouse", "Settings" };
int viewingMode = 0;
const String SettingsOptions[3]{ "Mouse Sensitivity", "Scroll Sensitivity", "Back" };
int settingsOption = 0;
const String viewingSettingsOptions[3]{ "Mouse Sensitivity", "Scroll Sensitivity", "Back" };
int viewingSettingsOption = 0;

//function pointer array for settings--------------------------------
typedef void (*settingsFunctions)();

void changeMouseSensitivity() {
  if (cnt % 20 == 0) {
    if(digitalRead(ru) == LOW){
      mouseSensitivity++;
      if(mouseSensitivity > 10){
        mouseSensitivity = 10;
      }
      settingsDirty = true;
    }
    if(digitalRead(rd) == LOW){
      mouseSensitivity--;
      if(mouseSensitivity < 1){
        mouseSensitivity = 1;
      }
      settingsDirty = true;
    }
    if(digitalRead(rs) == LOW){
      mode = 5;
    }
    printCenteredText("Mouse Sensitivity: " + String(mouseSensitivity), 1);
  }
}
void changeScrollSensitivity() {
  if (cnt % 20 == 0) {
    if(digitalRead(ru) == LOW){
      scrollSensitivity++;
      if(scrollSensitivity > 10){
        scrollSensitivity = 10;
      }
      settingsDirty = true;
    }
    if(digitalRead(rd) == LOW){
      scrollSensitivity--;
      if(scrollSensitivity < 1){
        scrollSensitivity = 1;
      }
      settingsDirty = true;
    }
    if(digitalRead(rs) == LOW){
      mode = 5;
    }
    printCenteredText("Scroll Sensitivity: " + String(scrollSensitivity), 1);
  }
}
void back() {
  if (settingsDirty) {
    saveSettingsToNVS();
  }
  mode = 0;                 // return to main menu
  settingsOption = 0;
}

const settingsFunctions settingsFunctionList[] = {
  changeMouseSensitivity,
  changeScrollSensitivity,
  back
};

//function pointer array for games--------------------------------
typedef void (*games)();

//dino game--------------------------------
void DinoGame() {
  
}
//tetris--------------------------------
void Tetris() {
  
}
//flappy bird--------------------------------


void FlappyBird() {
  if (!flappyStarted) {
    startFlappyGame();
  }

  if (flappyPaused) {
    drawFlappyGame(true);
    if (millis() >= flappyPauseUntil) {
      flappyPaused = false;
    }
    return;
  }

  if (flappyGameOver) {
    drawFlappyGameOver();
    if (!digitalRead(rs)) {
      startFlappyGame();
      delay(200);
    }
    if (!digitalRead(ls)) {
      mode = 0;
      viewingMode = 0;
      flappyStarted = false;
      flappyGameOver = false;
      delay(200);
    }
    return;
  }

  unsigned long now = millis();
  if (now - flappyLastFrame > 50) {
    flappyLastFrame = now;
    if (!digitalRead(ru) && flappyBirdY > 0) {
      flappyBirdVy = -6;
    }
    flappyBirdVy += 1;
    flappyBirdY += flappyBirdVy;
    if (flappyBirdY < 0) {
      flappyBirdY = 0;
      flappyBirdVy = 0;
    }
    for (int i = 0; i < 3; i++) {
      flappyPipes[i].x -= 4;
      if (flappyPipes[i].x < -6) {
        flappyPipes[i].x = 128;
        flappyPipes[i].gapY = random(12, 40);
        flappyScore++;
        if (flappyScore > flappyHighScore) {
          flappyHighScore = flappyScore;
          saveSettingsToNVS();
        }
      }
      if (flappyCollides(flappyPipes[i].x, flappyPipes[i].gapY, flappyPipes[i].gapHeight)) {
        flappyGameOver = true;
      }
    }
  }
  drawFlappyGame();
}

//mouse emulator--------------------------------
void MouseEmulator() {
  static unsigned long exitHoldStart = 0;
  if(digitalRead(rs) == LOW && digitalRead(ls) == LOW){
    if (exitHoldStart == 0) {
      exitHoldStart = millis();
    }
    if (millis() - exitHoldStart > 200) {
      mode = 0;
    }
  } else {
    exitHoldStart = 0;
  }

  if(!digitalRead(ru)){
    Mouse.move(0, -mouseSensitivity);
  }
  if(!digitalRead(rd)){
    Mouse.move(0, mouseSensitivity);
  }
  if(!digitalRead(rl)){
    Mouse.move(-mouseSensitivity, 0);
  }
  if(!digitalRead(rr)){
    Mouse.move(mouseSensitivity, 0);
  }
  if(!digitalRead(lu)){
    Mouse.move(0, 0, scrollSensitivity);
  }
  if(!digitalRead(ld)){
    Mouse.move(0, 0, -scrollSensitivity);
  }
  //left click--------------------------------
  if(!digitalRead(ll)){
    Mouse.press(MOUSE_LEFT);
  }
  else{
    Mouse.release(MOUSE_LEFT);
  }
  //right click--------------------------------
  if(!digitalRead(lr)){
    Mouse.press(MOUSE_RIGHT);
    delay(20);
    Mouse.release(MOUSE_RIGHT);
    delay(50);
  }
  printCenteredText("mouse active", 1);
}
//settings-----------------------------------------
void Settings() {
  if(mode == 5){
    if(cnt % 20 == 0){
      if (digitalRead(rs) == LOW) {
        settingsOption = viewingSettingsOption + 1;
      }
      if (digitalRead(rr) == LOW) {
        viewingSettingsOption++;
        if (viewingSettingsOption >= ARRAY_LENGTH(viewingSettingsOptions)) {
          viewingSettingsOption = 0;
        }
      }
      if (digitalRead(rl) == LOW) {
        viewingSettingsOption--;
        if (viewingSettingsOption < 0) {
          viewingSettingsOption = ARRAY_LENGTH(viewingSettingsOptions) - 1;
        }
      }
      if (settingsOption > 0) {
        settingsFunctionList[viewingSettingsOption]();
      }
      else {
        printCenteredText(viewingSettingsOptions[viewingSettingsOption], 1);
      }
    }
  }
}

const games gameList[] = {
  DinoGame,
  Tetris,
  FlappyBird,
  MouseEmulator,
  Settings
};

void setup() {
  bool i2c_init = Wire.begin(16, 17);
  if (!i2c_init) {
    Serial.println("I2C initialization failed");
    while (1)
      ;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
    ;
  }
  Mouse.begin();
  USB.begin();
  loadSettingsFromNVS();
  delay(500);
  display.setRotation(2);
  randomSeed(analogRead(1));
  drawLoadingScreen(random(1500, 2500));
}

void loop() {
  cnt++;
  delay(5);
  //-------------------------logic for changing states-----------------------------
  if (mode == 0) {
    if (cnt % 15 == 0) {
      if (digitalRead(rs) == LOW) {
        mode = viewingMode + 1;
      }
      if (digitalRead(rr) == LOW) {
        viewingMode++;
        if (viewingMode >= ARRAY_LENGTH(viewingModes)) {
          viewingMode = 0;
        }
      }
      if (digitalRead(rl) == LOW) {
        viewingMode--;
        if (viewingMode < 0) {
          viewingMode = ARRAY_LENGTH(viewingModes) - 1;
        }
      }
    }
  }
  //-------------------------------logic for games---------------------------------
  if (mode > 0) {
    gameList[mode - 1]();
  }
  //---------------------------------logic for printing screen----------------------------
  if (cnt % 4 == 0) {
    if(mode == 0){
      printCenteredText(viewingModes[viewingMode], 2);
    }
  }
}

void drawLoadingScreen(unsigned long durationMs) {
  unsigned long startTime = millis();
  unsigned long progressTime = 0;
  int barX = 14;
  int barY = 35;
  int barWidth = 100;
  int barHeight = 12;
  while (progressTime < durationMs) {
    progressTime = millis() - startTime;
    if (progressTime > durationMs) progressTime = durationMs;
    float t = (float)progressTime / durationMs;
    float eased = t * t * t;
    int percentage = eased * 100;
    int fillWidth = eased * barWidth;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(40, 20);
    display.print("LOADING...");
    display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);
    display.fillRect(barX + 2, barY + 2, fillWidth - 4 > 0 ? fillWidth - 4 : 0, barHeight - 4, SSD1306_WHITE);
    display.setCursor(53, 53);
    display.print(percentage);
    display.print("%");
    display.display();
    delay(15);
  }
  delay(500);
  display.clearDisplay();
  display.display();
}
void printCenteredText(String text, uint8_t size) {
  display.clearDisplay();
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  display.setCursor((128 - w) / 2, (64 - h) / 2);
  display.print(text);
  display.display();
}