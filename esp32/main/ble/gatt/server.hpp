#pragma once

#include <vector>
#include <memory>
#include "connection.hpp"
#include "service.hpp"

namespace ble::gatt {
class Server : public Connection {
  static constexpr const char *TAG = "Server$";

  std::shared_ptr<std::vector<std::unique_ptr<Service>>> services_ = {};

 public:
  Server(std::shared_ptr<std::vector<std::unique_ptr<Service>>> services)
      : services_(services) {}

  void OnConnect(esp_gatt_if_t gatt_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(TAG, "OnConnect Callled.");
    esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);

    this->gatt_if = gatt_if;
    this->conn_id = param->connect.conn_id;
  }

  void OnDisconnect(esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(TAG, "OnDisconnect Callled.");
    esp_ble_gap_start_advertising(&heart_rate_adv_params);
  }

  bool Write(uint16_t dest_handle, std::vector<uint8_t> &value,
             uint16_t offset = 0) {
    if (services_ == nullptr) {
      ESP_LOGE(TAG, "Services is null");
      return false;
    }
    for (auto &service : *services_) {
      if (service->OnWrite(dest_handle, value, offset)) {
        return true;
      }
    }
    return false;
  }
};

}  // namespace ble::gatt