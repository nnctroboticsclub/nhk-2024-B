#pragma once

#include <vector>
#include <memory>

#include "service.hpp"

namespace ble::gatt {
class Server {
  static constexpr const char *TAG = "Server$";

  std::shared_ptr<std::vector<std::unique_ptr<Service>>> services_ = {};

 public:
  Server(std::shared_ptr<std::vector<std::unique_ptr<Service>>> services)
      : services_(std::move(services)) {}

  void OnConnect(esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(TAG, "OnConnect Callled.");
    esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
  }

  void OnDisconnect(esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(TAG, "OnDisconnect Callled.");
    esp_ble_gap_start_advertising(&heart_rate_adv_params);
  }

  bool Write(uint16_t dest_handle, std::vector<uint8_t> &value,
             uint16_t offset = 0) {
    for (auto &service : *services_) {
      if (service->OnWrite(dest_handle, value, offset)) {
        return true;
      }
    }
    return false;
  }
};

}  // namespace ble::gatt