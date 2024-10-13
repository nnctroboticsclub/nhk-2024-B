idf_component_register(
  SRC_DIRS "."
  INCLUDE_DIRS "."
  REQUIRES bt esp_hid nvs_flash esp_wifi freertos
)

target_link_libraries(__idf_main PRIVATE
      syoch-robotics-logger
      syoch-robotics-timer
      syoch-robotics-gpio
      bd621x
      __idf_bt
)