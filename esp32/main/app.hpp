/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <memory>

#include "can_streaming.hpp"

#include <idf-robotics/can_driver.hpp>

#include <driver/uart.h>
#include "ble/ble.hpp"

/// Attributes State Machine

constexpr char *TAG = "CAN Debugger";

std::string ReadLine() {
  std::string line;
  while (true) {
    uint8_t c;

    auto ret = uart_read_bytes(UART_NUM_0, &c, 1, 1000 / portTICK_PERIOD_MS);
    if (ret == 1) {
      printf("[%02x]", c);
      if (c == '\n') {
        break;
      }
      line.push_back(c);
    }
  }
  return line;
}

void Console() {
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      //.rx_flow_ctrl_thresh = 122,
      //.use_ref_tick        = false,
  };

  auto ret = uart_param_config(UART_NUM_0, &uart_config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure UART");
    return;
  }

  ret = uart_driver_install(UART_NUM_0, 8192, 8192, 0, NULL, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to install UART driver");
    return;
  }

  while (true) {
    printf(">> ");
    fflush(stdout);

    auto cmd = ReadLine();

    if (cmd[0] == 'q') {
      break;
    } else if (cmd[0] == 'r') {
      auto count = esp_ble_get_bond_device_num();
      ESP_LOGI(TAG, "Bonded devices: %d", count);

      esp_ble_bond_dev_t *dev_list =
          (esp_ble_bond_dev_t *)malloc(count * sizeof(esp_ble_bond_dev_t));
      esp_ble_get_bond_device_list(&count, dev_list);

      for (int i = 0; i < count; i++) {
        ESP_LOGI(TAG, "Removing bond: %d", i);
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
      }

      free(dev_list);
    } else if (cmd[0] == 'b') {
      auto count = esp_ble_get_bond_device_num();
      ESP_LOGI(TAG, "Bonded devices: %d", count);

      esp_ble_bond_dev_t *dev_list =
          (esp_ble_bond_dev_t *)malloc(count * sizeof(esp_ble_bond_dev_t));
      esp_ble_get_bond_device_list(&count, dev_list);

      for (int i = 0; i < count; i++) {
        ESP_LOGI(TAG, "Bond: %d", i);
        ESP_LOGI(TAG, "  BD_ADDR: %02x:%02x:%02x:%02x:%02x:%02x",
                 dev_list[i].bd_addr[0], dev_list[i].bd_addr[1],
                 dev_list[i].bd_addr[2], dev_list[i].bd_addr[3],
                 dev_list[i].bd_addr[4], dev_list[i].bd_addr[5]);
      }

      free(dev_list);
    } else if (cmd[0] == 't') {
      printf("Test!\n");
    }
  }
}

extern "C" void app_main(void) {
  esp_err_t ret;

  ble::services::can_driver =
      std::make_shared<robotics::network::CANDriver>(GPIO_NUM_15, GPIO_NUM_4);
  ble::services::can_driver->Init();

  // Initialize NVS.
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  auto ble = ble::BLE::GetInstance();

  ble->gatt.AddService(std::make_unique<ble::services::CANStreamingService>());

  ble->Init();

  xTaskCreate(
      [](void *) -> void {
        Console();
        vTaskDelete(NULL);
      },
      "console", 4096, NULL, 5, NULL);

  /* Just show how to clear all the bonded devices
   * Delay 30s, clear all the bonded devices
   *
   * vTaskDelay(30000 / portTICK_PERIOD_MS);
   * remove_all_bonded_devices();
   */
}