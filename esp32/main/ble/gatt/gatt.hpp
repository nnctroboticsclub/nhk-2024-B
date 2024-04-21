#pragma once

#include <vector>
#include <memory>

#include <esp_gatts_api.h>

#include "profile.hpp"

namespace ble::gatt {
class GATT {
  static constexpr const char *TAG = "GATT!";
  std::vector<std::shared_ptr<GattsProfile>> prof_tab_ = {};
  const char *device_name = "ESP32";

 public:
  void AddProfile(std::shared_ptr<GattsProfile> profile) {
    prof_tab_.push_back(std::move(profile));
  }

  void SetDeviceName(const char *name) { device_name = name; }

  void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                           esp_ble_gatts_cb_param_t *param) {
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
      esp_bt_dev_set_device_name(device_name);
      esp_ble_gap_config_local_privacy(true);

      if (param->reg.status == ESP_GATT_OK) {
        /* for (auto profile : prof_tab_) {
          profile->gatts_if = gatts_if;
        } */
        prof_tab_[0]->gatts_if = gatts_if;
      } else {
        ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                 param->reg.app_id, param->reg.status);
        return;
      }
    }

    for (auto profile : prof_tab_) {
      if (gatts_if == ESP_GATT_IF_NONE || gatts_if == profile->gatts_if) {
        profile->CallCallback(event, gatts_if, param);
      }
    }
  }
};
}  // namespace ble::gatt