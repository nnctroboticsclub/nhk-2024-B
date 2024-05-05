#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include <esp_gatts_api.h>

#include "service.hpp"
#include "server.hpp"

#include "connection.hpp"

namespace ble::gatt {
class GATT : public Connections {
  static constexpr const char *TAG = "GATT!";
  std::shared_ptr<std::vector<std::unique_ptr<Service>>> services_;
  std::unordered_map<uint16_t, std::shared_ptr<Server>> servers_;
  const char *device_name = "ESP32";

 public:
  GATT()
      : services_(std::make_shared<std::vector<std::unique_ptr<Service>>>()),
        servers_() {}

  void Each(
      std::function<void(std::shared_ptr<Connection>)> callback) override {
    std::vector<uint16_t> conn_ids{};
    for (auto &&[conn_id, server] : servers_) {
      conn_ids.push_back(conn_id);
    }

    for (auto conn_id : conn_ids) {
      if (servers_.find(conn_id) == servers_.end()) {
        ESP_LOGE(TAG, "Server not found for conn_id %d", conn_id);
        continue;
      }

      callback(servers_[conn_id]);
    }
  }

  void AddService(std::unique_ptr<Service> profile) {
    if (!profile->initialized) {
      ESP_LOGW(TAG, "Service not initialized, initializing");
      profile->Init();
    } else {
      ESP_LOGW(TAG, "Service already initialized");
    }

    services_->push_back(std::move(profile));
  }

  void SetDeviceName(const char *name) { device_name = name; }

  void EventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                    esp_ble_gatts_cb_param_t *param) {
    switch (event) {
      case ESP_GATTS_REG_EVT: {
        esp_bt_dev_set_device_name(device_name);
        if (param->reg.status != ESP_GATT_OK) {
          ESP_LOGE(TAG, "Reg app failed, app_id %04x, status %d",
                   param->reg.app_id, param->reg.status);
          return;
        }

        int i = 0;
        for (auto &service : *services_) {
          ESP_LOGI(TAG, "Registering service %d", i);
          service->RegisterAttributeTable(i++, gatts_if);
        }
        break;
      }
      case ESP_GATTS_MTU_EVT:
        printf("ESP_GATTS_MTU_EVT: %d\n", param->mtu.mtu);
        break;

      case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
        ESP_LOGI(TAG, "The number handle = %x", param->add_attr_tab.num_handle);
        if (param->create.status == ESP_GATT_OK) {
          ESP_LOGI(TAG,
                   "Create attribute table successfully, the number "
                   "handle = %d",
                   param->add_attr_tab.num_handle);
          for (auto &service : *services_) {
            service->LoadHandles(&param->add_attr_tab);
          }
          esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
        } else {
          ESP_LOGE(TAG, " Create attribute table failed, error code = %x",
                   param->create.status);
        }
        break;
      }

      case ESP_GATTS_START_EVT:
        esp_ble_gap_start_advertising(&heart_rate_adv_params);
        break;

        //* Server events

      case ESP_GATTS_CONNECT_EVT: {
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT");

        esp_ble_conn_update_params_t conn_params = {};
        memcpy(conn_params.bda, param->connect.remote_bda,
               sizeof(esp_bd_addr_t));
        conn_params.latency = 0;
        conn_params.max_int = 0x20;  // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;  // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 1000;  // timeout = 1000*10ms = 10s
        esp_ble_gap_update_conn_params(&conn_params);

        auto server = std::make_unique<Server>(services_);
        server->OnConnect(gatts_if, param);

        servers_[param->connect.conn_id] = std::move(server);

        break;
      }

      case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x",
                 param->disconnect.reason);
        if (servers_.find(param->disconnect.conn_id) == servers_.end()) {
          ESP_LOGE(TAG, "Server not found for conn_id %d",
                   param->disconnect.conn_id);
          return;
        }
        servers_[param->disconnect.conn_id]->OnDisconnect(param);
        break;

      case ESP_GATTS_WRITE_EVT: {
        std::vector<uint8_t> data(param->write.value,
                                  param->write.value + param->write.len);
        if (servers_.find(param->write.conn_id) == servers_.end()) {
          ESP_LOGE(TAG, "Server not found for conn_id %d",
                   param->write.conn_id);
          return;
        }
        servers_[param->write.conn_id]->Write(param->write.handle, data,
                                              param->write.offset);
        break;
      }

      case ESP_GATTS_CONF_EVT:
        break;

      default:
        ESP_LOGE(TAG, "Unhandled GATT event %d", event);
    }
  }
};
}  // namespace ble::gatt