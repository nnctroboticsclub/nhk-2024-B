#pragma once

#include <vector>
#include <esp_gatts_api.h>

namespace ble::gatt {

static constexpr const char *TAG = "BLE/GATT";

struct AttrEntry {
  int id;
  esp_gatts_attr_db_t *db;

  uint16_t handle;

  AttrEntry(int id, esp_gatts_attr_db_t *db) : id(id), db(db), handle(-1) {}
};

class AttrDb {
  std::vector<esp_gatts_attr_db_t> db;

  std::vector<AttrEntry> entries;

  void AddService(int id, esp_gatts_attr_db_t entry) {
    for (auto &e : entries) {
      if (e.id == id) {
        ESP_LOGW(TAG, "Service with id %d already exists", id);
        e.db = &entry;
        return;
      }
    }

    entries.push_back(AttrEntry(id, &entry));
    db.push_back(entry);
  }

 public:
  enum class Perm : uint16_t {
    kRead = ESP_GATT_PERM_READ,
    kERead = ESP_GATT_PERM_READ_ENCRYPTED,
    kWrite = ESP_GATT_PERM_WRITE,
    kReadWrite = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    kEReadWrite = ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
  };

  template <typename T>
  void AddService(int id, uint16_t const &uuid, T &value, Perm perm) {
    esp_gatts_attr_db_t service_db = {{0}, {0, 0, 0, 0, 0, 0}};

    service_db.attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    service_db.att_desc.uuid_length = ESP_UUID_LEN_16;
    service_db.att_desc.uuid_p = (uint8_t *)&uuid;
    service_db.att_desc.perm = (uint16_t)perm;
    service_db.att_desc.max_length = sizeof(uint16_t);
    service_db.att_desc.length = sizeof(value);
    service_db.att_desc.value = (uint8_t *)&value;
    AddService(id, service_db);
  }

  void AddService(int id, uint16_t const &uuid, Perm perm) {
    esp_gatts_attr_db_t service_db = {{0}, {0, 0, 0, 0, 0, 0}};

    service_db.attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    service_db.att_desc.uuid_length = ESP_UUID_LEN_16;
    service_db.att_desc.uuid_p = (uint8_t *)&uuid;
    service_db.att_desc.perm = (uint16_t)perm;
    service_db.att_desc.max_length = sizeof(uint16_t);
    service_db.att_desc.length = 0;
    service_db.att_desc.value = nullptr;
    AddService(id, service_db);
  }

  AttrEntry *GetEntry(int id) {
    for (auto &e : entries) {
      if (e.id == id) {
        return &e;
      }
    }

    return nullptr;
  }

  void LoadHandles(uint16_t *handles, size_t count) {
    for (size_t i = 0; i < count; i++) {
      GetEntry(i)->handle = handles[i];
    }
  }
  const std::vector<esp_gatts_attr_db_t> GetDb() const { return db; }
};
}  // namespace ble::gatt