#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
#define TOUCH_CS_PIN D3
#define TOUCH_IRQ_PIN D2

#define TS_MINX 330
#define TS_MINY 213
#define TS_MAXX 3963
#define TS_MAXY 3890

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
XPT2046_Touchscreen ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN);

// The display also uses hardware SPI(D5,D6,D7), SD3, D4, D8
static uint8_t SD3 = 10;
#define TFT_CS SD3
#define TFT_DC D4
#define BL_LED D8

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

long int answer = 0;
char lastchar = ' ';
String key1;
String key2;
int key1i = 0;
int key2i = 0;
char operation = ' ';
/*long Time = 0;
  long millicount = 0;
  int interval = 1000;
  int screenTimout = 15;*/
bool beentouched = false;
bool toLong = false;
bool equal = false;

#define row1x 0
#define boxsize 48

#define r1x 144
#define extray 48

int x, y = 0;


char button[4][4] = {
  { '7', '8', '9', '/' },
  { '4', '5', '6', 'X' },
  { '1', '2', '3', '-' },
  { 'C', '0', '=', '+' }
};

void draw()
{
  tft.fillScreen(ILI9341_BLACK);

  tft.drawRoundRect(row1x, extray, boxsize, boxsize, 8, ILI9341_WHITE);
  tft.drawRoundRect(row1x, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
  tft.drawRoundRect(row1x, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
  tft.drawRoundRect(row1x, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_RED);

  for (int b = extray; b <= 192; b += boxsize)
  {
    tft.drawRoundRect  (row1x + boxsize, b, boxsize, boxsize, 8, ILI9341_WHITE);
    tft.drawRoundRect  (row1x + boxsize * 3, b, boxsize, boxsize, 8, ILI9341_BLUE);
  }
  tft.drawRoundRect(row1x + boxsize * 2, extray, boxsize, boxsize, 8, ILI9341_WHITE);
  tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
  tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
  tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);

  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      tft.setCursor(16 + (boxsize * i), extray + 12 + (boxsize * j));
      tft.setTextSize(3);
      tft.setTextColor(ILI9341_WHITE);
      tft.println(button[j][i]);
    }
  }
  tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
  tft.setCursor(4, 12);
}

void setup()
{
  key1 = "";
  key2 = "";
  pinMode(BL_LED, OUTPUT);
  digitalWrite(BL_LED, HIGH);
  Serial.begin(9600);
  Serial.println("Calculator");
  ts.begin();
  tft.begin();
  tft.setRotation(3);
  draw();
  tft.setCursor(4, 12);
}

void loop()
{
  if (ts.touched() && !beentouched)
  {
    if (equal)
    {
      equal = false;
      draw();
    }

    if (key1.length() + key2.length() + int(operation != ' ') > 16)
    {
      toLong = true;
    }

    TS_Point p = ts.getPoint();     // Read touchscreen
    beentouched = true;

    x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

    lastchar = ' ';
    lastchar = idbutton();

    //If input is number
    if (lastchar >= '0' && lastchar <= '9' && !toLong)
    {
      //If input is digitr & operation is not defined
      if (operation == ' ')
      {
        key1 += lastchar;
        tft.print(lastchar);
        Serial.println(key1);
      }

      //If input is digit & operation is defined
      else
      {
        /*TODO*/
        //You need to determine how ke1, key2, operation will change in every TODO.
        //Write your code here.
        key2 += lastchar;
        /*TODO*/
        tft.print(lastchar);
        Serial.println(key2);
      }
    }

    //If input is an operator
    if ((lastchar == '+' || lastchar == '-' || lastchar == '/' || lastchar == 'X') && key2 == "" && key1 != "" && !toLong)
    {
      if ( operation != ' ')
      {
        operation = lastchar;
        tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
        tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
        tft.setCursor(4, 12);
        tft.print(key1);
        tft.print(operation);
      }

      //If input is an operator and operation is not defined
      else
      {
        /*TODO*/
        //You need to determine how ke1, key2, operation will change in every TODO.
        //Write your code here.
        operation = lastchar;
        /*TODO*/
        tft.print(operation);
      }
    }

    //If input is equal
    if (lastchar == '=')
    {
      //If neither key1 or key2 are empty
      if (key1 != "" && key2 != "")
      {
        equal = true;
        Serial.println("Calculate");
        //Check devide by zero error
        if (key2.toInt() == 0 && operation == '/')
        {
          tft.setCursor(4, 12);
          tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
          tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
          tft.print("=");
          tft.setTextColor(ILI9341_RED);
          tft.print("ERROR");
          tft.setTextColor(ILI9341_WHITE);
          key1 = "";
          key2 = "";
          operation = ' ';
        }
        else
        {
          key1i = 0;
          key2i = 0;
          key1i = key1.toInt();
          key2i = key2.toInt();
          answer = calc(key1i, key2i, operation);

          tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
          tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
          tft.setCursor(4, 12);
          tft.print('=');
          tft.print(answer);
          key1i = 0;
          key2i = 0;
          operation = ' ';
          key1 = "";
          key2 = "";
        }
      }
    }

    if (lastchar == 'C')
    {
      key1 = "";
      key2 = "";
      answer = 0;

      key1i = 0;
      key2i = 0;

      operation = ' ';
      draw();
      tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
      tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
      tft.setCursor(4, 12);
      toLong = false;
    }

    //wait for release
    while (ts.touched()) {
      delay(10);
    };
    beentouched = false;
  }
}

char idbutton()
{
  //Row 1 identification
  if ((x >= 0) && (x <= boxsize))
  {
    //    Serial.println("Row 1  ");
    //7
    if (((extray + boxsize) >= y) && (y >= extray))
    {
      tft.drawRoundRect(row1x, extray, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x, extray, boxsize, boxsize, 8, ILI9341_WHITE);
      return '7';
    }
    //4
    if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
    {
      tft.drawRoundRect(row1x, extray + boxsize, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
      return '4';
    }
    //1
    if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
    {
      tft.drawRoundRect(row1x, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
      return '1';
    }
    //C
    if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
    {
      tft.drawRoundRect(row1x, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_WHITE);
      delay(100);
      tft.drawRoundRect(row1x, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_RED);
      return 'C';
    }

  }

  //Row 2 identification
  if ((x >= boxsize) && (x <= (boxsize * 2)))
  {
    //    Serial.println("Row 2  ");
    //8
    if (((extray + boxsize) >= y) && (y >= extray))
    {
      tft.drawRoundRect(row1x + boxsize, extray, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x + boxsize, extray, boxsize, boxsize, 8, ILI9341_WHITE);
      return '8';
    }
    //5
    if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
    {
      tft.drawRoundRect(row1x + boxsize, extray + boxsize, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x + boxsize, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
      return '5';
    }
    //2
    if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
    {
      tft.drawRoundRect(row1x + boxsize, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x + boxsize, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
      return '2';
    }
    //0
    if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
    {
      tft.drawRoundRect(row1x + boxsize, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x + boxsize, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_WHITE);
      return '0';
    }
  }

  //Row 3 identification
  if ((x >= (boxsize * 2)) && (x <= (boxsize * 3)))
  {
    //    Serial.println("Row 3  ");
    //9
    if (((extray + boxsize) >= y) && (y >= extray))
    {
      tft.drawRoundRect(row1x + boxsize * 2, extray, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 2, extray, boxsize, boxsize, 8, ILI9341_WHITE);
      return '9';
    }
    //6
    if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
    {
      tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
      return '6';
    }
    //3
    if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
    {
      tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_RED);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
      return '3';
    }
    //=
    if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
    {
      tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_BLUE);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);
      return '=';
    }

  }

  //Row 4 identification
  if ((x >= (boxsize * 3)) && (x <= (boxsize * 4)))
  {
    //    Serial.println("Row 4  ");
    //+
    if (((extray + boxsize) >= y) && (y >= extray))
    {
      tft.drawRoundRect(row1x + boxsize * 3, extray, boxsize, boxsize, 8, ILI9341_GREEN);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 3, extray, boxsize, boxsize, 8, ILI9341_BLUE);
      return '/';
    }
    //-
    if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
    {
      tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize, boxsize, boxsize, 8, ILI9341_GREEN);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize, boxsize, boxsize, 8, ILI9341_BLUE);
      return 'X';
    }
    //*
    if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
    {
      tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_GREEN);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_BLUE);
      return '-';
    }
    // /
    if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
    {
      tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);
      delay(100);
      tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_BLUE);
      return '+';
    }
  }
}

long int calc(long int num1, long int num2, char op)
{
  /*TODO*/
  //Write your code here.
  //This part is to calculate the answer
  //You can refer to the example in the slide.
  if (op == '+') return  num1 + num2;
  if (op == 'X') return  num1 * num2;
  if (op == '-') return  num1 - num2;
  if (op == '/') return  num1 / num2;
  /*TODO*/
}




