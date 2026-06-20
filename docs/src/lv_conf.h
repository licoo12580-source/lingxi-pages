/**
 * @file lv_conf.h
 * Configuration for LVGL v9
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* 颜色深度: TFT_eSPI使用16位RGB565 */
#define LV_COLOR_DEPTH 16

/* 使用自定义显示驱动 (TFT_eSPI) */
#define LV_USE_TFT_ESPI 0

/* 内存设置 */
#define LV_MEM_SIZE (64 * 1024)           /* 64KB */
#define LV_MEM_POOL_INCLUDE <stdlib.h>
#define LV_MEM_POOL_ALLOC malloc

/* 分辨率 */
#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 480

/* 使用内置字体 */
#define LV_FONT_MONTSERRAT_8  1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_34 1
#define LV_FONT_MONTSERRAT_40 0

#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* 关闭不需要的功能以减少Flash占用 */
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 0
#define LV_USE_ASSERT_MALLOC 0
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_FREETYPE 0
#define LV_USE_THORVG 0
#define LV_USE_RLOTTIE 0
#define LV_USE_PNG 0
#define LV_USE_BMP 0
#define LV_USE_SJPG 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0
#define LV_USE_BARCODE 0
#define LV_USE_IMGFONT 0
#define LV_USE_OBSERVER 1

/* 需要的组件 */
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_LINE 1
#define LV_USE_FLEX 1
#define LV_USE_GRID 1
#define LV_USE_SLIDER 0
#define LV_USE_SWITCH 0
#define LV_USE_CHART 0
#define LV_USE_TABLE 0
#define LV_USE_DROPDOWN 1
#define LV_USE_ROLLER 0
#define LV_USE_TEXTAREA 1
#define LV_USE_TILEVIEW 0
#define LV_USE_CONTAINER 0
#define LV_USE_LIST 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_SPAN 0
#define LV_USE_KEYBOARD 0

/* 动画 */
#define LV_USE_ANIMATION 1

/* 主题 */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 1

/* 触摸 */
#define LV_INDEV_DEF_PRESSURE_LIMIT 50
#define LV_INDEV_DEF_LONG_PRESS_TIME 400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME 100

/* LVGL 官方 demo */
#define LV_USE_DEMO_WIDGETS 1
#define LV_USE_DEMO_BENCHMARK 0

/* demo需要的额外组件 */
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_CHART 1
#define LV_USE_LED 1
#define LV_USE_SPINNER 1
#define LV_USE_SPINBOX 1
#define LV_USE_METER 1
#define LV_USE_WIN 1
#define LV_USE_TABVIEW 1
#define LV_USE_CHECKBOX 1
#define LV_USE_KEYBOARD 1
#define LV_USE_TEXTAREA 1
#define LV_USE_DROPDOWN 1
#define LV_USE_ROLLER 1
#define LV_USE_LIST 1
#define LV_USE_TABLE 1
#define LV_USE_CALENDAR 1
#define LV_USE_MSGBOX 1
#define LV_USE_IMG 1
#define LV_USE_CANVAS 1

#endif /* LV_CONF_H */
