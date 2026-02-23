#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// User provided pins
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define TFT_CS 5
#define TFT_DC 4
#define TFT_RST 3
#define TFT_MOSI 22
#define TFT_SCLK 18
// Note: MISO is not used for ST7789 usually, but defined if needed.
#define TFT_MISO -1
#define TFT_BL -1 // Backlight NOT defined by user, assume none or handled externally

class LGFX_ESP32C6_ST7789 : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX_ESP32C6_ST7789()
    {
        {
            auto cfg = _bus_instance.config();

            // SPI bus config
            cfg.spi_host = SPI2_HOST; // ESP32-C6 SPI2
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000; // 40MHz
            cfg.freq_read = 16000000;
            cfg.spi_3wire = true; // ST7789 is often 3-wire SPI (MOSI/SCLK/CS/DC)
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = TFT_SCLK;
            cfg.pin_mosi = TFT_MOSI;
            cfg.pin_miso = TFT_MISO;
            cfg.pin_dc = TFT_DC;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs = TFT_CS;
            cfg.pin_rst = TFT_RST;
            cfg.pin_busy = -1;

            cfg.panel_width = SCREEN_WIDTH;
            cfg.panel_height = SCREEN_HEIGHT;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = true; // Often true for IPS ST7789
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;

            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};
