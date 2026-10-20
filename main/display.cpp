#include "display.h"

#include "bsp/esp-bsp.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_lcd_panel_ops.h"

namespace display {

namespace {

constexpr const char *TAG = "display";

// The panel is portrait; turned 90 degrees, it is landscape. The adapter rotates each frame with the PPA while
// copying it into one of the panel's three frame buffers (CONFIG_BSP_LCD_DPI_BUFFER_NUMS), so the panel never
// shows a half-drawn frame.
constexpr esp_lv_adapter_rotation_t ROTATION = ESP_LV_ADAPTER_ROTATE_90;
constexpr esp_lv_adapter_tear_avoid_mode_t TEAR_AVOID = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;

} // namespace

lv_display_t *start()
{
    // Powers the DSI PHY, opens the I2C bus (backlight and touch) and initializes the OTA7290B.
    esp_lcd_panel_handle_t panel = nullptr;
    esp_lcd_panel_io_handle_t io = nullptr;
    ESP_ERROR_CHECK(bsp_display_new(nullptr, &panel, &io));
    ESP_ERROR_CHECK(bsp_display_brightness_set(0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    const esp_lv_adapter_config_t adapter = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter));

    esp_lv_adapter_display_config_t config =
        ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(panel, io, BSP_LCD_H_RES, BSP_LCD_V_RES, ROTATION);
    config.tear_avoid_mode = TEAR_AVOID;
    config.profile.enable_ppa_accel = true;
    config.profile.use_psram = true;
    lv_display_t *display = esp_lv_adapter_register_display(&config);
    ESP_ERROR_CHECK(display ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(esp_lv_adapter_start());

    ESP_LOGI(TAG, "%ldx%ld landscape on a %dx%d panel", static_cast<long>(lv_display_get_horizontal_resolution(display)),
             static_cast<long>(lv_display_get_vertical_resolution(display)), BSP_LCD_H_RES, BSP_LCD_V_RES);
    return display;
}

void set_brightness(int percent)
{
    // Over I2C: a failure only leaves the brightness where it was.
    if (bsp_display_brightness_set(percent) != ESP_OK) ESP_LOGW(TAG, "brightness not set");
}

Lock::Lock() { esp_lv_adapter_lock(-1); }

Lock::~Lock() { esp_lv_adapter_unlock(); }

} // namespace display
