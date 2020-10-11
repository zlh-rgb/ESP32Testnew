#include <Arduino.h>
#include "bno080/SparkFun_BNO080_Arduino_Library.h" 
#include <Adafruit_ILI9341.h>
#include "Ads_112c04/Ads_112c04.h"
#include "touchScreen/pa_touchScreen.h"
extern "C"
{
#include "bno055/bno055.h"
}
#include "pa_miniGUI/pa_button/pa_Button.h"
#define TFT_CS 13
#define TFT_DC 12
#define TFT_RST 14

#define Attitude 1
#define Ads1292 2
#define Lm70 3
#define TempOffset 204.393
#define Normal 0
#define Fever 1
#define HighFever 2

namespace Btn
{
  extern pa_Button Btn_switch;
  extern pa_Button Btn_ads;
  extern pa_Button Btn_lm70;
  extern pa_Button Btn_attitude;

  void adsCallback();
  void lm70Callback();
  void attitudeCallback();
  void switchCallback();
}; 

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

char buffer[20];
BNO080 myIMU;
bno055_vector_t myBNO055;
float bno055_InitZ = 0;
unsigned char a = 50;
unsigned char humanBodyState = 0;
int stepCount = 0;
double temperature = 0;
double temperatureAverage = 0;
double initialTemperature = 0;
double handleTemperature[20];
double temperatureSum = 0;
int heartRateValue = 0;
Ads_112c04 &ads = Ads_112c04::instance;

unsigned char Attitude_cache[180] = {0};
unsigned char Ads1292_cache[180] = {0};
unsigned char Lm70_cache[180] = {0};

void waveformDisplay(int16_t x, int16_t y, double data, double dataMax, double dataMin, unsigned char subsection, unsigned device); //画坐标位置 27<=x<=62

bool pa_Button::isPressed()
{
  return pa_touchScreen::instance.isPressed();
}
namespace GUI
{
  void gui_Pos()
  {
    tft.setCursor(50, 17);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.printf("Attitude\n");
    tft.println();

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.printf("X:%.2lf\n", myBNO055.x);
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.printf("Y:%.2lf\n", myBNO055.y);
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.printf("Z:%.2lf\n", myBNO055.z);
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.printf("stepCount:%d\n", stepCount);
    tft.drawRoundRect(160, 10, 50, 30, 4, ILI9341_WHITE);
    tft.setCursor(174, 17);
    tft.setTextColor(ILI9341_WHITE);
    tft.printf(">>", stepCount);
    waveformDisplay(33, 151, myBNO055.z, 180, 0, 5, Attitude);
  }
  void gui_ads1292()
  {
    tft.setCursor(50, 17);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.printf("ADS1292\n");
    tft.println();

    tft.drawRoundRect(160, 10, 50, 30, 4, ILI9341_WHITE);
    tft.setCursor(174, 17);
    tft.setTextColor(ILI9341_WHITE);
    tft.printf(">>", stepCount);
    waveformDisplay(33, 151, myBNO055.z, 1800, 0, 5, Ads1292);
  }
  void gui_lm70()
  {
    tft.setCursor(70, 17);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.printf("LM70\n");
    tft.println();
    tft.printf("Temp:\n");
    tft.println();
    tft.printf("State:");
    switch (humanBodyState)
    {
    case Normal:
      tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
      tft.setCursor(70, 52);
      tft.printf("%.2f\n",temperature);
      tft.setCursor(78, 82);
      tft.printf("Health    ");
      break;
    case Fever:
      tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
      tft.setCursor(70, 52);
      tft.printf("%.2f\n",temperature);
      tft.setCursor(78, 82);
      tft.printf("Fever    ");
      break;
    case HighFever:
      tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
      tft.setCursor(70, 52);
      tft.printf("%.2f\n",temperature);
      tft.setCursor(78, 82);
      tft.printf("HighFever");
      break;
    }
    
    tft.drawRoundRect(160, 10, 50, 30, 4, ILI9341_WHITE);
    tft.setCursor(174, 17);
    tft.setTextColor(ILI9341_WHITE);
    tft.printf(">>", stepCount);
    waveformDisplay(33, 151, temperature, 45, 20, 5, Lm70);

  }
  void drawButton(int x, int y, int w, int h, const char *str)
  {
    tft.drawRoundRect(x, y, w, h, 4, ILI9341_WHITE);
    tft.setCursor(x + 10, y + 5);
    tft.setTextColor(ILI9341_WHITE);
    tft.printf(str, stepCount);
  }
  void gui_Menu()
  {
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(90, 30);
    tft.printf("Menu");
    drawButton(60, 60, 120, 30, "Ads1292");
    drawButton(60, 120, 120, 30, "Lm70");
    drawButton(60, 180, 120, 30, "Attitude");
  }
#define GUI_chosen_menu 0
#define GUI_chosen_pos 1
#define GUI_chosen_ads1292 2
#define GUI_chosen_lm70 3
  int GUI_Chosen = 0;

  void loop()
  {
    if (GUI_Chosen == GUI_chosen_menu)
    {
      Btn::Btn_ads.loop();
      Btn::Btn_attitude.loop();
      Btn::Btn_lm70.loop();
    }
    else
    {
      Btn::Btn_switch.loop();
    }
    switch (GUI_Chosen)
    {
    case GUI_chosen_pos:
      gui_Pos();
      break;
    case GUI_chosen_menu:
      gui_Menu();
      break;
    case GUI_chosen_lm70:
      gui_lm70();
      break;
    case GUI_chosen_ads1292:
      gui_ads1292();
      break;
    }
  }
} 

namespace Btn
{
  pa_Button Btn_switch;
  pa_Button Btn_ads;
  pa_Button Btn_lm70;
  pa_Button Btn_attitude;

  void adsCallback()
  {
    Serial.println("ads");
    GUI::GUI_Chosen = GUI_chosen_ads1292;
    tft.fillScreen(ILI9341_BLACK);
  }
  void lm70Callback()
  {
    Serial.println("lm70");
    GUI::GUI_Chosen = GUI_chosen_lm70;
    tft.fillScreen(ILI9341_BLACK);
  }
  void attitudeCallback()
  {
    Serial.println("attitude");
    GUI::GUI_Chosen = GUI_chosen_pos;
    tft.fillScreen(ILI9341_BLACK);
  }
  void switchCallback()
  {
    Serial.println("switch");
    GUI::GUI_Chosen = GUI_chosen_menu;
    tft.fillScreen(ILI9341_BLACK);
  }
} 

void setup()
{
  pinMode(5,OUTPUT);
  digitalWrite(5,HIGH);
  Serial.begin(115200);
  Serial.println();
  Serial.println("HELLO");
  pa_touchScreen::instance.init(240, 320, 260, 3800, 425, 3780, 100);

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);

  Wire.begin();

  // pa_BNO055_init();

  ads.init(Ads_112c04::AxState::DGND, Ads_112c04::AxState::DGND);
  ads.configRegister0(Ads_112c04::Gain::GAIN_1);
  delay(100);
  ads.configRegister1(Ads_112c04::SpeedOfSample::SPS_1000, Ads_112c04::Mode::Mode_Normal, Ads_112c04::ConvMode::Continuous);
  ads.startConv();

  Wire.setClock(400000); //Increase I2C data rate to 400kHz

  Btn::Btn_switch.init(160, 10, 50, 30);
  Btn::Btn_ads.init(60, 60, 120, 30);
  Btn::Btn_lm70.init(60, 120, 120, 30);
  Btn::Btn_attitude.init(60, 180, 120, 30);

  Btn::Btn_ads.buttonCallback = Btn::adsCallback;
  Btn::Btn_attitude.buttonCallback = Btn::attitudeCallback;
  Btn::Btn_lm70.buttonCallback = Btn::lm70Callback;
  Btn::Btn_switch.buttonCallback = Btn::switchCallback;
}

void loop()
{

  uint16_t coord[2];
  pa_touchScreen::instance.readRaw(coord);
  pa_touchScreen::instance.turnRawToScreen(coord);
  pa_Button::setPos(coord[0], coord[1]);

  double adc = ads.readADC();//获取112c04的值
  adc = (float)adc * (2.048 / 32768) * 1000;
  // Serial.printf("%F \r\n",adc);
  initialTemperature = (-0.0000084515) * adc * adc + (-0.176928) * adc + TempOffset;//数据拟合
  //均值滤波
  static unsigned char j = 0;
  handleTemperature[j] = initialTemperature;
  j++;
  j%=20;
  temperatureSum = 0;
  for (unsigned char i = 0; i < 20; i++)
  {
    temperatureSum += handleTemperature[i];
  }
  temperature = temperatureSum/20;
  //温度判断人状态
  if (temperature>=37.3 && temperature<38.3)
  {
    humanBodyState = Fever;
  }
  else if (temperature>=38.3)
  {
    humanBodyState = HighFever;
  }
  else
  {
    humanBodyState = Normal;
  }
  //获取055角度值
  myBNO055 = pa_BNO055_getVector();
  if (a)
  {
    bno055_InitZ = myBNO055.z;
    --a;
  }
  //计算步数
  stepCount = bno055_StepCount(myBNO055);
  //获取心率值
  if(Serial.read()!=-1)
  {
    heartRateValue = Serial.read();
    Serial.printf("%d",heartRateValue);
  }
  //gui
  GUI::loop();
}

//波形显示
void waveformDisplay(int16_t x, int16_t y, double data, double dataMax, double dataMin, unsigned char subsection, unsigned device)
{
  static unsigned char Attitude_count = 0;
  static unsigned char Ads1292_count = 0;
  static unsigned char Lm70_count = 0;

  unsigned char i;

  //画坐标系、标刻度
  tft.drawLine(x, y - 8, x, y + 2 + 90, ILI9341_BLUE);            //竖轴
  tft.drawLine(x, y + 2 + 90, x + 180, y + 2 + 90, ILI9341_BLUE); //横轴
  for (i = 0; i < subsection + 1; i++)
  {
    tft.setCursor(x - 27, y - 2 + i * (90 / subsection));
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(1);
    tft.printf("%.0f", (dataMax - dataMin) / subsection * (subsection - i) + dataMin);
    tft.drawLine(x - 2, y + i * (90 / subsection), x, y + i * (90 / subsection), ILI9341_WHITE); //标刻度
  }
  //更新数据线
  switch (device)
  {
  case Attitude:
    Attitude_cache[Attitude_count] = (unsigned char)abs((data- dataMin) * 90 / (dataMax -dataMin));
    tft.drawLine(x + Attitude_count + 1 , y, x + Attitude_count + 1, y + 90, ILI9341_BLUE);                                                                       //蓝色前刷新线
    tft.drawLine(x + Attitude_count, y, x + Attitude_count, y + 90, ILI9341_BLACK);                                                                              //黑色后刷新线
    tft.drawLine(x + Attitude_count, y + 90 - Attitude_cache[Attitude_count - 1], x + Attitude_count + 1, y + 90 - Attitude_cache[Attitude_count], ILI9341_RED); //数据线  241~151
    tft.drawLine(x + 180, y, x + 180, y + 90, ILI9341_BLACK); //数据线  241~151
    Attitude_count++;
    Attitude_count %= 180;
    break;
  case Ads1292:
    Ads1292_cache[Ads1292_count] = (unsigned char)abs((data- dataMin) * 90 / (dataMax -dataMin));
    tft.drawLine(x + Ads1292_count + 1 , y, x + Ads1292_count + 1, y + 90, ILI9341_BLUE);                                                                       //蓝色前刷新线
    tft.drawLine(x + Ads1292_count, y, x + Ads1292_count, y + 90, ILI9341_BLACK);                                                                              //黑色后刷新线
    tft.drawLine(x + Ads1292_count, y + 90 - Ads1292_cache[Ads1292_count - 1], x + Ads1292_count + 1, y + 90 - Ads1292_cache[Ads1292_count], ILI9341_RED); //数据线  241~151
    tft.drawLine(x + 180, y, x + 180, y + 90, ILI9341_BLACK);//数据线  241~151
    Ads1292_count++;
    Ads1292_count %= 180;
    break;
  case Lm70:
    Lm70_cache[Lm70_count] = (unsigned char)abs((data- dataMin) * 90 / (dataMax -dataMin));
    tft.drawLine(x + Lm70_count + 1 , y, x + Lm70_count + 1, y + 90, ILI9341_BLUE);                                                                       //蓝色前刷新线
    tft.drawLine(x + Lm70_count, y, x + Lm70_count, y + 90, ILI9341_BLACK);
    switch (humanBodyState)
    {
    case Normal:
      tft.drawLine(x + Lm70_count, y + 90 - Lm70_cache[Lm70_count - 1], x + Lm70_count + 1, y + 90 - Lm70_cache[Lm70_count], ILI9341_GREEN); 
      break;
    case Fever:
      tft.drawLine(x + Lm70_count, y + 90 - Lm70_cache[Lm70_count - 1], x + Lm70_count + 1, y + 90 - Lm70_cache[Lm70_count], ILI9341_YELLOW);
      break;
    case HighFever:
      tft.drawLine(x + Lm70_count, y + 90 - Lm70_cache[Lm70_count - 1], x + Lm70_count + 1, y + 90 - Lm70_cache[Lm70_count], ILI9341_RED);
      break;
    }                                                                             
    tft.drawLine(x + 180, y, x + 180, y + 90, ILI9341_BLACK);
    Lm70_count++;
    Lm70_count %= 180;
    break;
  }
}