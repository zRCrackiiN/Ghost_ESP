#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include "board_config.h"

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

#ifdef DISPLAY_SUPPORT
#include <TFT_eSPI.h>
#include "lvgl.h"
#include <XPT2046_Touchscreen.h>
#include <LinkedList.h>
#include <core/images/bt.h>
#include <core/images/logo.h>


inline XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

struct SplashScreen
{
    const char* Text;
    int Progress;
    const char* lastSplashText = "";
    int lastSplashProgress = -1;
    lv_obj_t* splash_screen;
    lv_obj_t* splash_label;
    lv_obj_t* splash_image;
    lv_obj_t* splash_progress_bar;
};

enum MenuType
{
    MT_MainMenu,
    MT_BluetoothMenu,
    MT_WifiUtilsMenu,
    MT_LEDUtils,
};

struct Card {
    int x, y, w, h;
    String title;
    uint16_t bgColor;
    uint16_t fgColor;
    bool isSelected;
    const unsigned char* imageBuffer;
    const int imagebuffersize;
    lv_obj_t* Object;
};

const int numCards = 3;
const int cardWidth = 50;
const int cardHeight = 50;
const int cardSpacing = 15;
const int xOffset = 80;      // Starting x-offset from left of the screen, adjust based on preference
const int yOffset = (240 - cardHeight) / 2;  // Center cards height-wise

class DisplayModule {
public:
    SPIClass mySpi = SPIClass(HSPI);
    uint16_t backgroundColor, buttonColor, textColor, buttonTextColor;
    bool IsOnSplash;
    MenuType mtype;
    TFT_eSPI tft = TFT_eSPI();
    SplashScreen Splash;
    uint8_t* draw_buf;
    lv_indev_t * indev;

    lv_obj_t* lv_main_menu;
    lv_obj_t *label_obj;
    Card cards[numCards] = {
        {xOffset, yOffset, cardWidth, cardHeight, "BLE Attacks", 0, 0, false, bt_jpg, bt_jpg_size},
        {xOffset + cardWidth + cardSpacing, yOffset, cardWidth, cardHeight, "WiFi Utils", 0, 0, false, nullptr, 0},
        {xOffset + 2 * (cardWidth + cardSpacing), yOffset, cardWidth, cardHeight, "Led Utils", 0, 0, false, nullptr, 0}
    };

    DisplayModule() : backgroundColor(0), buttonColor(0),
                      textColor(0),
                      buttonTextColor(0) {
    }
    int LastTouchX;
    int LastTouchY;
    void drawMainMenu();
    void TouchPadRead();
    void drawSelectedLabel(String &label);
    void animateMenu();
    void animateCardPop(Card &card);
    void drawCard(Card &card);
    void setButtonCallback(int buttonIndex, void (*callback)());
    void checkTouch(int tx, int ty);
    void UpdateSplashStatus(const char* Text, int Percent);
    void Init();
    void RenderJpg(int x, int y, int w = 0, int h = 0);
    void printTouchToSerial(TS_Point p);
};

#endif
#endif