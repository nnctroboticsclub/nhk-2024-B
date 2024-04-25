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

#include "ble/ble.hpp"

/// Attributes State Machine

constexpr char *TAG = "CAN Debugger";

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

  /* Just show how to clear all the bonded devices
   * Delay 30s, clear all the bonded devices
   *
   * vTaskDelay(30000 / portTICK_PERIOD_MS);
   * remove_all_bonded_devices();
   */
}