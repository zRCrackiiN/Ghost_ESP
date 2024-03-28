#include "display_module.h"
#ifdef DISPLAY_SUPPORT

#include <JPEGDecoder.h>

void DisplayModule::RenderJpg(int xpos, int ypos, int w, int h, Card* card) {
    static lv_obj_t* img_obj = nullptr;

    if(img_obj == nullptr) {
        img_obj = lv_img_create(lv_scr_act());
    }

    lv_img_set_src(img_obj, &card ? card->imageBuffer : logo_jpg);
    lv_obj_set_pos(img_obj, xpos, ypos); 

    
    if(w > 0 && h > 0) {
        lv_obj_set_size(img_obj, w, h);
        lv_img_set_antialias(img_obj, true);
    }
}

void DisplayModule::UpdateSplashStatus(const char* Text, int Percent)
{
    lv_obj_t* splash_screen = Splash.splash_screen;
    lv_obj_t* splash_label = Splash.splash_label;
    lv_obj_t* splash_image = Splash.splash_image;
    lv_obj_t* splash_progress_bar = Splash.splash_progress_bar;

   
    if(splash_screen == nullptr) {
        splash_screen = lv_obj_create(NULL);
        lv_obj_set_size(splash_screen, 240, 320);
        lv_obj_clear_flag(splash_screen, LV_OBJ_FLAG_SCROLLABLE);

       
        if(logo_jpg != nullptr && splash_image == nullptr) {
            splash_image = lv_img_create(splash_screen);
            lv_img_set_src(splash_image, logo_jpg);
            lv_obj_align(splash_image, LV_ALIGN_CENTER, 0, -50);
        }

        
        splash_label = lv_label_create(splash_screen);
        lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 50);

        
        splash_progress_bar = lv_bar_create(splash_screen);
        lv_obj_set_size(splash_progress_bar, 200, 20); 
        lv_obj_align(splash_progress_bar, LV_ALIGN_BOTTOM_MID, 0, -30);
        lv_bar_set_range(splash_progress_bar, 0, 100);
    }

    
    if(Text != nullptr) {
        lv_label_set_text(splash_label, Text);
    }

    
    if(splash_progress_bar != nullptr) {
        lv_bar_set_value(splash_progress_bar, Percent, LV_ANIM_OFF);
    }

    
    if (Percent > 99) {
        delay(1000);
        lv_obj_del_async(splash_screen);
        splash_screen = nullptr;
        splash_label = nullptr;
        splash_image = nullptr;
        splash_progress_bar = nullptr;
        drawMainMenu();
    }
}

void DisplayModule::drawMainMenu()
{
    lv_main_menu = lv_obj_create(NULL);
    lv_obj_set_size(lv_main_menu, LV_HOR_RES, LV_VER_RES);

    for (int i = 0; i < numCards; i++) {
        drawCard(cards[i]);
    }
}

void DisplayModule::animateMenu()
{

}

void DisplayModule::setButtonCallback(int buttonIndex, void (*callback)()) {
   
}

void DisplayModule::animateCardPop(const Card &card) {
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, card.Object);
    lv_anim_set_time(&a, 150);
    lv_anim_set_playback_time(&a, 150);
    lv_anim_set_playback_delay(&a, 0);

    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) { lv_obj_set_width((lv_obj_t*)obj, v); });
    lv_anim_set_values(&a, lv_obj_get_width(card.Object), lv_obj_get_width(card.Object) + 8); // Increase width
    lv_anim_start(&a);

    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) { lv_obj_set_height((lv_obj_t*)obj, v); });
    lv_anim_set_values(&a, lv_obj_get_height(card.Object), lv_obj_get_height(card.Object) + 8); // Increase height
    lv_anim_start(&a);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_border_color(&style, lv_color_make(255, 0, 0)); // Red border
    lv_style_set_border_width(&style, 2);
    lv_obj_add_style(card.Object, &style, 0);
}

void DisplayModule::checkTouch(int tx, int ty) {
    if (LastTouchX != tx && LastTouchY != ty)
    {
        for (int i = 0; i < numCards; i++) {
           
            if (tx > cards[i].x && tx < cards[i].x + cards[i].w && ty > cards[i].y && ty < cards[i].y + cards[i].h) {
                cards[i].isSelected = true;
                drawCard(cards[i]);
                drawSelectedLabel(cards[i].title);
                LastTouchX = tx;
                LastTouchY = ty;
            } 
            else 
            {
                cards[i].isSelected = false;
                drawCard(cards[i]);
                LastTouchX = tx;
                LastTouchY = ty;
            }
        }
    }
}

void DisplayModule::drawSelectedLabel(const String &label) {
    if(label_obj == nullptr) {
        label_obj = lv_label_create(lv_scr_act());
        lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(label_obj, 240);
        lv_obj_align(label_obj, LV_ALIGN_TOP_LEFT, 5, 10);
    }


    lv_label_set_text(label_obj, label.c_str());


    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);
    lv_obj_add_style(label_obj, &style, 0);
}

void DisplayModule::drawCard(Card &card) {
    if (card.imageBuffer != nullptr) {
        RenderJpg(card.x, card.y, card.w, card.h, &card);
    }
    else 
    {
        card.Object = lv_obj_create(lv_scr_act());

        lv_obj_set_pos(card.Object, card.x, card.y);

        lv_obj_set_size(card.Object, card.w, card.h);
        
        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_bg_opa(&style, LV_OPA_COVER);
        lv_style_set_bg_color(&style, lv_color_make(card.bgColor >> 16, (card.bgColor >> 8) & 0xFF, card.bgColor & 0xFF));
        lv_obj_add_style(card.Object, &style, 0);
    }
}

void DisplayModule::Init()
{
    IsOnSplash = true;
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin();
    ts.setRotation(1);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
}

void DisplayModule::printTouchToSerial(TS_Point p) {
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x);
  Serial.print(", y = ");
  Serial.print(p.y);
  Serial.println();
}

#endif