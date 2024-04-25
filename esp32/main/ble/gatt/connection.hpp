#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

#include <esp_gatts_api.h>

namespace ble::gatt {
struct Connection {
  esp_gatt_if_t gatt_if;
  uint16_t conn_id;

  void Notify(uint16_t handle, std::vector<uint8_t> value) {
    esp_ble_gatts_send_indicate(gatt_if, conn_id, handle, value.size(),
                                value.data(), false);
  }

  void Notify(uint16_t handle, uint8_t *value, size_t length) {
    esp_ble_gatts_send_indicate(gatt_if, conn_id, handle, length, value, false);
  }
};

class Connections {
 public:
  virtual void Each(
      std::function<void(std::shared_ptr<Connection>)> callback) = 0;
};
}  // namespace ble::gatt