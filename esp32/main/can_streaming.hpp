#pragma once

#include <cstdint>
#include <esp_log.h>
#include <idf-robotics/can_driver.hpp>

#include "ble/gatt/service.hpp"
#include "ble/ble.hpp"

namespace ble::services {

const std::uint8_t service_uuid[16] = {0x7e, 0x5c, 0xfd, 0x53, 0x1f, 0xe5,
                                       0x82, 0x91, 0x19, 0x4d, 0xda, 0x51,
                                       0x3e, 0xf2, 0xb6, 0x20};

const std::uint8_t bus_load_uuid[16] = {0x4c, 0x1f, 0x5e, 0x79, 0xb7, 0x5b,
                                        0x7e, 0xbc, 0x7b, 0x46, 0x7e, 0xcb,
                                        0x41, 0x1d, 0xef, 0x59};
std::uint16_t bus_load_val = 0;

struct BusPacket {
  std::uint16_t op;
  std::uint8_t len;
  std::uint8_t data[8];
} __attribute__((packed));

const std::uint8_t bus_rx_uuid[16] = {0xe8, 0x29, 0xe1, 0x89, 0x53, 0x74,
                                      0x04, 0xa2, 0xfb, 0x4e, 0x3a, 0x61,
                                      0xb2, 0x87, 0xff, 0x3e};
BusPacket bus_rx_val;

const std::uint8_t bus_tx_uuid[16] = {0x4e, 0x9d, 0x73, 0xd5, 0x17, 0x62,
                                      0xda, 0x81, 0x04, 0x4c, 0x1a, 0xd6,
                                      0xf8, 0x7f, 0x07, 0x18};
BusPacket bus_tx_val;

std::shared_ptr<robotics::network::CANDriver> can_driver = nullptr;

class CANStreamingService : public ble::gatt::Service {
  static constexpr const char *TAG = "Profile#CAN";
  enum attrs {
    kIdxSvc,
  };

  ble::gatt::Characteristic *rx_char = nullptr;

 public:
  void Init() override {
    this->SetServiceUUID(service_uuid);

    this->AddCharacteristic(
        &(new ble::gatt::Characteristic())
             ->SetUUID(bus_load_uuid)
             .SetValue(bus_load_val)
             .SetPermissions(ble::gatt::Attribute::Perm::kRead)
             .SetProperties(ble::gatt::chararacteristic::Prop::kNotify)  //
             .SetMaxValueLength(2)
        /* .AddDescriptor((new ble::gatt::Attribute())
                            ->SetValue((std::uint8_t &)heart_measurement_ccc)
                            .SetUUID(ESP_GATT_UUID_CHAR_CLIENT_CONFIG)
                            .SetMaxValueLength(2)
                            .SetPermissions(
                                ble::gatt::Attribute::Perm::kReadWrite)  //
                       )                                                 // */
    );

    rx_char =
        &(new ble::gatt::Characteristic())
             ->SetUUID(bus_rx_uuid)
             .SetPermissions(ble::gatt::Attribute::Perm::kERead)         //
             .SetProperties(ble::gatt::chararacteristic::Prop::kNotify)  //
             .SetValue(bus_rx_val);

    can_driver->OnRx([this](std::uint16_t op, std::vector<std::uint8_t> data) {
      bus_rx_val.op = op;
      bus_rx_val.len = data.size();
      std::copy(data.begin(), data.end(), bus_rx_val.data);

      std::stringstream ss;
      ss << "Received CAN packet: op=" << op << ", len=" << data.size()
         << ", data=";
      for (auto &byte : data) {
        ss << " " << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
      }
      ESP_LOGI(TAG, "%s", ss.str().c_str());

      ble::BLE::GetInstance()->gatt.Each(
          [this](std::shared_ptr<ble::gatt::Connection> conn) {
            if (!conn) {
              ESP_LOGW(TAG, "Connection is null");
              return;
            }
            conn->Notify(rx_char->GetValueAttr().GetHandle(),
                         (uint8_t *)&bus_rx_val, sizeof(BusPacket));
          });
    });

    this->AddCharacteristic(rx_char);

    this->AddCharacteristic(
        &(new ble::gatt::Characteristic())
             ->SetUUID(bus_tx_uuid)
             .SetPermissions(ble::gatt::Attribute::Perm::kEReadWrite)     //
             .SetProperties(ble::gatt::chararacteristic::Prop::kWriteNR)  //
             .SetValue(bus_tx_val)                                        //
             .SetOnWriteCallback(
                 [](std::vector<uint8_t> data, uint16_t offset) {
                   if (data.size() < sizeof(BusPacket)) {
                     ESP_LOGE(TAG, "Invalid data size %d", data.size());
                     return;
                   }

                   esp_log_buffer_hex(TAG, data.data(), data.size());

                   bus_tx_val = *(BusPacket *)data.data();
                   if (can_driver) {
                     std::vector<uint8_t> data = {
                         bus_tx_val.data, bus_tx_val.data + bus_tx_val.len};
                     can_driver->Send(bus_tx_val.op, data);
                   }
                 })  //
    );
  }
};
}  // namespace ble::services