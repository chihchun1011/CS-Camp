#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <string.h>
#include "buttonCoordinate.h"
#define TOUCH_CS_PIN D3
#define TOUCH_IRQ_PIN D2 

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 309
#define TS_MINY 184
#define TS_MAXX 3658
#define TS_MAXY 3928

XPT2046_Touchscreen ts(TOUCH_CS_PIN,TOUCH_IRQ_PIN);

// The display also uses hardware SPI, plus #9 & #10
static uint8_t SD3 = 10;
#define TFT_CS SD3
#define TFT_DC D4
#define BL_LED D8

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

ButtonCoordinate startButton, giveupButton, reverseButton, reguessButton;
ButtonCoordinate numButtons[10];

enum GameState{startMode, gameMode, reverseMode, endMode};
GameState gameState;

struct GameData {
    char R[5];
    char guess[5];
    bool histR[10];
    bool histG[10];
    int A;
    int B;
    int count;
} gameData;

struct RevData {
    bool possible[10000];
    int guess;
    int A;
    int B;
    int count;
    int remain;
} revData;

void resetGuess() {
    strcpy(gameData.guess, "xxxx");
    for(int i = 0; i < 10; ++i) {
        gameData.histG[i] = 0;
    }
    gameData.A = -1;
    gameData.B = -1;
}

void resetGameData() {
    resetGuess();
    strcpy(gameData.R, "    ");
    for(int i = 0; i < 10; ++i) {
        gameData.histR[i] = 0;
    }
    gameData.count = 0;
}

void resetRevGuess() {
    revData.A = -1;
    revData.B = -1;
    revData.guess = 0;
}

void resetRevData() {
    Serial.print("resetRevData()");
    revData.remain = 10000;

    // remove all invalid number
    // also check that there are 5040 remains 

    /* TODO lecture5
     *  your code here
     */

    Serial.print("remain: ");
    Serial.print(revData.remain);
    Serial.print("\n");
    resetRevGuess();
    revData.count = 0;
}

void setup() {
    pinMode(BL_LED,OUTPUT);
    digitalWrite(BL_LED, HIGH);
    Serial.begin(9600);
    Serial.print("Starting...");
    randomSeed(millis());
 
    if (!ts.begin()) {
        Serial.println("Couldn't start touchscreen controller");
        while (1);
    }

    menuSetup();
    initDisplay();
    drawStartScreen();
}

void menuSetup() {
    gameState = startMode;
    startButton = ButtonCoordinate(30,180,100,40);
    reverseButton  = ButtonCoordinate(150,180,140,40);
}

void gameSetup() {
    Serial.print("gameSetup\n");
    giveupButton = ButtonCoordinate(192,0,128,56);
    for (int i = 0; i < 10; ++i) {
        numButtons[i] = ButtonCoordinate((i % 5) * 64, 112 + (i / 5) * 64, 64, 64);
    }
    resetGameData();
    generate();
    gameState = gameMode;
    Serial.print("finish gameSetup\n");
}

void initDisplay() {
    tft.begin();
    tft.setRotation(3);
}

void drawStartScreen() {
    tft.fillScreen(BLACK);

    //Draw white frame
    tft.drawRect(0,0,319,240,WHITE);
    
    tft.setCursor(20,100);
    tft.setTextColor(WHITE);
    tft.setTextSize(4);
    tft.print("Guess Number");
    
    tft.setCursor(80,30);
    tft.setTextColor(GREEN);
    tft.setTextSize(4);
    tft.print("Arduino");

    createStartButton();
}

void createStartButton() {
    startButton.fillAndDraw(tft, RED, WHITE);
    tft.setCursor(36,188);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.print("Start");
    
    reverseButton.fillAndDraw(tft, RED, WHITE);
    tft.setCursor(156,188);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.print("Reverse");
}

void generate() {
    /* TODO lecture4
     *  your code here
     */
    Serial.print("generate R: ");
    Serial.print(gameData.R);
    Serial.print("\n");
}

void loop() {
    bool isTouched = ts.touched();
    if(isTouched) {
        isTouched = false;
        TS_Point p0 = ts.getPoint();  //Get touch point
        TS_Point p;
        p.x = map(p0.x, TS_MINX, TS_MAXX, 0, 320);
        p.y = map(p0.y, TS_MINY, TS_MAXY, 0, 240);

        Serial.print("X = "); Serial.print(p.x);
        Serial.print("\tY = "); Serial.print(p.y);
        Serial.print("\n");

        if(startButton.pressed(p.x, p.y)) {
            gameSetup();
            execGame();
        }
        else if(reverseButton.pressed(p.x, p.y)) {
            revGameSetup();
            execRevGame();
        }
    }
    if(gameState == endMode) {
        drawStartScreen();
        gameState = startMode;
    }
}

void execGame() {
    Serial.print("execGame\n");
    while(gameState == gameMode) {
        drawGameScreen();
        playerMove();
        checkGuess();
    }
}

void drawVerticalLine(int x) {
    for(int i=0;i<5;i++) {
        tft.drawLine(x+i,112,x+i,240,WHITE);
    }
}

void drawHorizontalLine(int y) {
    for(int i=0;i<5;i++) {
        tft.drawLine(0,y+i,320,y+i,WHITE);
    }
}

void printNumber(int index) {
    // char* num_str[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    int offset = 16;
    // int x = offset + (index % 5) * 64;
    // int y = offset + 112 + (index / 5) * 64;
    int x = offset + numButtons[index].x;
    int y = offset + numButtons[index].y;
    tft.setCursor(x, y);
    tft.print(index);
}

void drawGameScreen() {
    tft.fillScreen(BLACK);
    tft.drawRect(0,0,319,240,WHITE);
    for(int i = 0; i < 4; ++i) {
        drawVerticalLine(64 * (i+1) - 2);
    }
    for(int i = 0; i < 4; ++i) {
        drawHorizontalLine(112 + 64 * i - 2);
    }
    for(int i = 0; i < 10; ++i) {
        if(!gameData.histG[i]) printNumber(i);
    }
    tft.setCursor(192 + 8, 0 + 8);
    tft.print("giveup");
    // draw guess
    tft.setCursor(8, 8);
    tft.print(gameData.guess);
    // draw result
    if(gameData.A == 4) {
        tft.setCursor(8, 74 + 8);
        tft.print("Bingo!");
    }
    else {
        tft.setCursor(8, 37 + 8);
        tft.print("A: ");
        if(gameData.A >= 0) tft.print(gameData.A);
        tft.setCursor(8, 74 + 8);
        tft.print("B: ");
        if(gameData.B >= 0) tft.print(gameData.B);
    }
    tft.setCursor(128 + 8, 74 + 8);
    tft.print("count: ");
    tft.print(gameData.count);
}

void playerMove() {
    bool isTouched = false;
    int count = 0;
    while(count < 4) {
        isTouched = ts.touched();
        if(isTouched) {
            isTouched = false;
            if(count == 0) resetGuess();
            TS_Point p0 = ts.getPoint();  //Get touch point
            TS_Point p;
            p.x = map(p0.x, TS_MINX, TS_MAXX, 0, 320);
            p.y = map(p0.y, TS_MINY, TS_MAXY, 0, 240);
            Serial.print("X = "); Serial.print(p.x);
            Serial.print("\tY = "); Serial.print(p.y);
            Serial.print("\n");
            if(giveupButton.pressed(p.x, p.y)) {
                gameState = endMode;
                break;
            }
            for(int i = 0; i < 10; ++i) {
                if(numButtons[i].pressed(p.x, p.y)) {
                    if(!gameData.histG[i]) {
                        /* TODO lecture4
                         *  your code here
                         */
                    }
                    Serial.print("count = ");
                    Serial.print(count);
                    Serial.print("\tguess = ");
                    Serial.print(gameData.guess);
                    Serial.print("\n");
                }
            }
        }
        delay(50); // prevent wdt reset and continuous input
    }
    ++gameData.count;
}

void checkGuess() {
    Serial.print("checkGuess\n");
    
    /* TODO lecture4
     *  your code here
     */

    if(gameData.A == 4) {
        gameState = endMode;
        drawGameScreen();
        delay(2000);
    }
    for(int i = 0; i < 10; ++i) {
        gameData.histG[i] = 0;
    }
}

// Reverse mode: AI guess your number

void revGameSetup() {
    giveupButton = ButtonCoordinate(192,0,128,37);
    reguessButton = ButtonCoordinate(160,37,128,37);
    for (int i = 0; i < 10; ++i) {
        numButtons[i] = ButtonCoordinate((i % 5) * 64, 112 + (i / 5) * 64, 64, 64);
    }
    resetRevData();
    randomSeed(millis());
    gameState = reverseMode;
}

void execRevGame() {
    Serial.print("execRevGame\n");
    while(gameState == reverseMode) {
        AIMove();
        drawRevScreen();
        revPlayerMove();
        AIFilter();
    }
}

void drawRevScreen() {
    tft.fillScreen(BLACK);
    tft.drawRect(0,0,319,240,WHITE);
    for(int i = 0; i < 4; ++i) {
        drawVerticalLine(64 * (i+1) - 2);
    }
    for(int i = 0; i < 4; ++i) {
        drawHorizontalLine(112 + 64 * i - 2);
    }
    for(int i = 0; i < 5; ++i) {
        printNumber(i);
    }
    tft.setCursor(192 + 8, 0 + 8);
    tft.print("back");
    // draw guess
    tft.setCursor(8, 8);
    if(revData.remain > 0) {
        if(revData.guess < 1000) tft.print("0");
        tft.print(revData.guess);
    }
    else {
        tft.print("Impossible!");
    }
    // draw result
    if(revData.A == 4) {
        tft.setCursor(8, 37 + 8);
        tft.print("Game Over!");
    }
    else {
        tft.setCursor(160 + 8, 37 + 8);
        tft.print("reGuess");
        tft.setCursor(8, 37 + 8);
        tft.print("A: ");
        if(revData.A >= 0) tft.print(revData.A);
        tft.setCursor(8, 74 + 8);
        tft.print("B: ");
        if(revData.B >= 0) tft.print(revData.B);
    }
    tft.setCursor(128 + 8, 74 + 8);
    tft.print("count: ");
    tft.print(revData.count);
}

void AIMove() {
    // AI guess a number
    resetRevGuess();
    
    /* TODO lecture5
     *  your code here
     */

    ++revData.count;
}

void revPlayerMove() {
    // player enter result
    bool isTouched = false;
    int count = 0;
    while(count < 2) {
        isTouched = ts.touched();
        if(isTouched) {
            isTouched = false;
            TS_Point p0 = ts.getPoint();  //Get touch point
            TS_Point p;
            p.x = map(p0.x, TS_MINX, TS_MAXX, 0, 320);
            p.y = map(p0.y, TS_MINY, TS_MAXY, 0, 240);
            Serial.print("X = "); Serial.print(p.x);
            Serial.print("\tY = "); Serial.print(p.y);
            Serial.print("\n");
            if(giveupButton.pressed(p.x, p.y)) {
                gameState = endMode;
                break;
            }
            if(reguessButton.pressed(p.x, p.y)) {
                AIMove();
                --revData.count;
                drawRevScreen();
            }
            for(int i = 0; i < 5; ++i) {
                if(numButtons[i].pressed(p.x, p.y)) {
                    if(count == 0) revData.A = i;
                    else if(count == 1) revData.B = i;
                    ++count;
                    drawRevScreen();
                }
            }
            if(revData.A == 4) {
                gameState = endMode;
                delay(2000);
                break;
            }
        }
        delay(50); // prevent wdt reset and continuous input
    }
    ++gameData.count;
}

void AIFilter() {
    Serial.print("filter\n");
    // AI filters out possibilities by player result
    if(gameState != reverseMode) return;
    
    /* TODO lecture5
     *  your code here
     */

    Serial.print("remain: ");
    Serial.print(revData.remain);
    Serial.print("\n");
    if(revData.remain == 0) {
        drawRevScreen();
        gameState = endMode;
        delay(2000);
    }
    else {
        for(int i = 0; i < 10000; ++i) {
            if(revData.possible[i]) {
                Serial.print("first possible: ");
                Serial.print(i);
                Serial.print("\n");
                break;
            }
        }
    }
}

