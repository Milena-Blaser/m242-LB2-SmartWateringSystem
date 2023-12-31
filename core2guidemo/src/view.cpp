#include "view.h"

uint32_t startTime, frame = 0; // For frames-per-second estimate
TFT_eSPI tft = TFT_eSPI();
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];


static lv_style_t button_disabled_style;


void init_m5() {
  M5.begin(true, true, true, true);
  tft.begin();
  tft.setRotation(1);
  M5.Axp.SetLcdVoltage(2800);
  M5.Axp.SetLcdVoltage(3300);
  M5.Axp.SetBusPowerMode(0);
  M5.Axp.SetCHGCurrent(AXP192::kCHG_190mA);
  M5.Axp.SetLDOEnable(3, true);
  delay(150);
  M5.Axp.SetLDOEnable(3, false);
  M5.Axp.SetLed(1);
  delay(100);
  M5.Axp.SetLed(0);
  M5.Axp.SetLDOVoltage(3, 3300);
  M5.Axp.SetLed(1);
  startTime = millis();
}

bool my_touchpad_read(lv_indev_drv_t * indev_driver,
                      lv_indev_data_t * data){
  TouchPoint_t pos = M5.Touch.getPressPoint();
  bool touched = ( pos.x == -1 ) ? false : true;
  if(!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR; 
    /*Set the coordinates*/
    data->point.x = pos.x;
    data->point.y = pos.y;
  }
  return false; 
//Return `false` because we are not buffering and no more data to read
}


void my_disp_flush(lv_disp_drv_t *disp, 
     const lv_area_t *area, lv_color_t *color_p){
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(&color_p->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

void init_display() {
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
  lv_init();
  
  
  //-------------------------------------------------------------------
  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res  = 320;
  disp_drv.ver_res  = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer   = &disp_buf;
  lv_disp_drv_register(&disp_drv);
  
  //-------------------------------------------------------------------
  /*Initialize the (dummy) input device driver*/
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  lv_style_init(&button_disabled_style);
  lv_style_set_border_color(&button_disabled_style, LV_STATE_DISABLED, LV_COLOR_GRAY);
  lv_style_set_text_color(&button_disabled_style, LV_STATE_DISABLED, LV_COLOR_GRAY);
}

lv_obj_t * add_label(const char * text, lv_coord_t x_ofs, lv_coord_t y_ofs) {
  lv_obj_t * label = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(label, text);
  lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, x_ofs, y_ofs);

  return label;
}


// https://docs.lvgl.io/7.11/widgets/btn.html#overview
lv_obj_t * add_button(const char * text, lv_event_cb_t event_cb, lv_coord_t x_ofs, lv_coord_t y_ofs) {
    lv_obj_t * label;
    lv_obj_t * btn1 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn1, event_cb);

    static lv_style_t style_halo;
    lv_style_init(&style_halo);
    lv_style_set_transition_time(&style_halo, LV_STATE_PRESSED, 400);
    lv_style_set_transition_time(&style_halo, LV_STATE_DEFAULT, 0);
    lv_style_set_transition_delay(&style_halo, LV_STATE_DEFAULT, 200);
    lv_style_set_outline_width(&style_halo, LV_STATE_DEFAULT, 0);
    lv_style_set_outline_width(&style_halo, LV_STATE_PRESSED, 20);
    lv_style_set_outline_opa(&style_halo, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_outline_opa(&style_halo, LV_STATE_FOCUSED, LV_OPA_COVER);   /*Just to be sure, the theme might use it*/
    lv_style_set_outline_opa(&style_halo, LV_STATE_PRESSED, LV_OPA_TRANSP);
    lv_style_set_transition_prop_1(&style_halo, LV_STATE_DEFAULT, LV_STYLE_OUTLINE_OPA);
    lv_style_set_transition_prop_2(&style_halo, LV_STATE_DEFAULT, LV_STYLE_OUTLINE_WIDTH);

    lv_obj_align(btn1, NULL, LV_ALIGN_IN_BOTTOM_LEFT, x_ofs, y_ofs);
    lv_obj_add_style(btn1, LV_OBJ_PART_ALL, &button_disabled_style);
    lv_obj_refresh_style(btn1, LV_STYLE_PROP_ALL);

    label = lv_label_create(btn1, NULL);

    lv_label_set_text(label, text);

    lv_obj_add_style(btn1, LV_BTN_PART_MAIN, &style_halo);

    return btn1;
}

static void slider_event_cb(lv_obj_t * slider, lv_event_t event);
static lv_obj_t * slider_label;

lv_obj_t * add_slider(int min, int max, lv_coord_t x_ofs, lv_coord_t y_ofs){
    /* Create a slider in the center of the display */
    lv_obj_t * slider = lv_slider_create(lv_scr_act(), NULL);
    lv_obj_set_width(slider, LV_DPI * 2);
    lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_event_cb(slider, slider_event_cb);
    lv_slider_set_range(slider, min, max);
    
    /* Create a label below the slider */
    slider_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(slider_label, "0");
    lv_obj_set_auto_realign(slider_label, true);
    lv_label_set_recolor(slider_label, true);
    lv_obj_align(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    return slider;
}

 void slider_event_cb(lv_obj_t * slider, lv_event_t event){
    if(event == LV_EVENT_VALUE_CHANGED) {
        static char buf[4]; /* max 3 bytes for number plus 1 null terminating byte */
        snprintf(buf, 4, "%u", lv_slider_get_value(slider));
        lv_label_set_text(slider_label, buf);
    }
}

lv_obj_t * show_message_box(const char * text, const char * ok_button_text, const char * no_button_text, lv_event_cb_t event)
{
    static const char * btns[] ={ok_button_text, no_button_text, ""};

    lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
    lv_msgbox_set_text(mbox1, text);
    lv_msgbox_add_btns(mbox1, btns);
    lv_obj_set_width(mbox1, 200);
    lv_obj_set_event_cb(mbox1, event);
    lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); 

    return mbox1;
}

void close_message_box(lv_obj_t * msgbox) {
  lv_msgbox_start_auto_close(msgbox, 800);
}


lv_obj_t * show_message_box_no_buttons(const char * text)
{
    lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
    lv_msgbox_set_text(mbox1, text);
    lv_obj_set_width(mbox1, 200);
    lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); 
    return mbox1;
}

// https://docs.lvgl.io/7.11/widgets/led.html
lv_obj_t * add_led(lv_coord_t x_ofs, lv_coord_t y_ofs, lv_coord_t w, lv_coord_t h)
{
  lv_obj_t * led1  = lv_led_create(lv_scr_act(), NULL);
  lv_obj_align(led1, NULL, LV_ALIGN_IN_TOP_LEFT, x_ofs, y_ofs);
  lv_obj_set_width(led1, w);
  lv_obj_set_height(led1, h);
  lv_led_off(led1);
  return led1;
}

// https://docs.lvgl.io/7.11/widgets/checkbox.html#simple-checkbox
lv_obj_t * add_checkbox(const char * text, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_event_cb_t event_handler)
{
    lv_obj_t * cb = lv_checkbox_create(lv_scr_act(), NULL);
    lv_checkbox_set_text(cb, text);
    lv_obj_align(cb, NULL, LV_ALIGN_IN_TOP_LEFT, x_ofs, y_ofs);
    if(event_handler != NULL) {
      lv_obj_set_event_cb(cb, event_handler);
    }

    return cb;
}