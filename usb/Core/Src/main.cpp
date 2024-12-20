/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#include <stdio.h>

#include "main.h"
#include "usb_host.h"
#include "usbh_hid.h"

#include <chrono>

#include <srobo2/ffi/base.hpp>
#include <srobo2/com/im920.hpp>
#include <srobo2/com/im920_srobo1.hpp>
#include <srobo2/timer/hal_timer.hpp>

#include <ssp/ssp.hpp>
#include <ssp/value_store.hpp>
#include <ssp/keep_alive.hpp>

#include <nhk2024b/ps4_con.hpp>
#include <robotics/types/joystick_2d.hpp>

#include <nhk2024b/ps4_vs.hpp>
#include <nhk2024b/robot1/controller.hpp>
#include <nhk2024b/robot2/controller.hpp>
#include <nhk2024b/node_id.hpp>
#include <logger.h>
#include <im920.h>
UART_HandleTypeDef huart2;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);

static void MX_USART2_UART_Init(void);
extern "C" void MX_USB_HOST_Process(void);

static bool is_usb_hid_connected = false;
static bool is_controller_stopped = false;

namespace robotics::system {
void SleepFor(std::chrono::milliseconds duration) {
  HAL_Delay(duration.count());
}
}  // namespace robotics::system

namespace board_led {
const int kPin1 = GPIO_PIN_7;
const int kPin2 = GPIO_PIN_8;
const int kPin3 = GPIO_PIN_9;

inline void On(uint16_t pin) { HAL_GPIO_WritePin(GPIOC, pin, GPIO_PIN_SET); }

inline void Off(uint16_t pin) { HAL_GPIO_WritePin(GPIOC, pin, GPIO_PIN_RESET); }

inline void Toggle(uint16_t pin) { HAL_GPIO_TogglePin(GPIOC, pin); }
}  // namespace board_led

/**
 * @brief  The application entry point.
 * @retval int
 */
extern "C" int main(void) {
  setbuf(stdout, NULL);

  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USB_HOST_Init();

  printf("\n\n\nProgram started\n");

  robotics::logger::core::Init();
  nhk2024b::controller::im920::Init();
  vs_ps4::state::Init();

  using robotics::network::ssp::SerialServiceProtocol;
  using robotics::network::ssp::ValueStoreService;

  auto im920 = nhk2024b::controller::im920::GetIM920();
  auto nn = im920->GetNodeNumber();
  auto gn = im920->GetGroupNumber();
  auto ch = im920->GetChannel();

  SerialServiceProtocol<uint16_t, bool> ssp(*im920);
  auto vs = ssp.RegisterService<ValueStoreService<uint16_t, bool>>();
  auto keep_alive = ssp.RegisterService<
      robotics::network::ssp::KeepAliveService<uint16_t, bool>>();

  auto pipe1_remote = nhk2024b::node_id::GetPipe1Remote(nn);
  auto robot1_ctrl = new nhk2024b::robot1::Controller();
  vs_ps4::stick_left >> robot1_ctrl->move;
  vs_ps4::dpad >> robot1_ctrl->buttons;
  vs_ps4::button_share >> robot1_ctrl->emc;
  vs_ps4::trigger_l >> robot1_ctrl->rotation_ccw;
  vs_ps4::trigger_r >> robot1_ctrl->rotation_cw;
  robot1_ctrl->RegisterTo(vs, pipe1_remote);
  keep_alive->AddTarget(pipe1_remote);

  auto pipe2_remote = nhk2024b::node_id::GetPipe2Remote(nn);
  auto robot2_ctrl = new nhk2024b::robot2::Controller();
  vs_ps4::stick_right >> robot2_ctrl->move;
  vs_ps4::button_options >> robot2_ctrl->emc;
  vs_ps4::button_cross >> robot2_ctrl->button_deploy;
  vs_ps4::button_square >> robot2_ctrl->button_bridge_toggle;
  vs_ps4::button_circle >> robot2_ctrl->button_unassigned0;
  vs_ps4::button_triangle >> robot2_ctrl->button_unassigned1;
  vs_ps4::button_l1 >> robot2_ctrl->test_decrease;
  vs_ps4::button_r1 >> robot2_ctrl->test_increase;

  vs_ps4::button_ps.SetChangeCallback([](bool btn) {
    is_controller_stopped ^= btn;
    if (is_controller_stopped) {
      board_led::On(board_led::kPin3);
    } else {
      board_led::Off(board_led::kPin3);
    }
    vs_ps4::state::trigger_l_value = 0;
    vs_ps4::state::trigger_r_value = 0;
  });

  robot2_ctrl->RegisterTo(vs, pipe2_remote);
  keep_alive->AddTarget(pipe2_remote);

  //* Visualize node_number, group_number, channel
  board_led::Off(board_led::kPin1);
  board_led::Off(board_led::kPin2);
  board_led::Off(board_led::kPin3);
  for (int i = 0; i < 64; i++) {
    if ((nn >> (i % 32)) & 1) board_led::Toggle(board_led::kPin1);
    if ((gn >> (i % 16)) & 1) board_led::Toggle(board_led::kPin2);
    if ((ch >> (i % 8)) & 1) board_led::Toggle(board_led::kPin3);

    HAL_Delay(5);
  }

  board_led::Off(board_led::kPin1);
  board_led::Off(board_led::kPin2);
  board_led::Off(board_led::kPin3);

  printf("Entering Main loop\n");

  const float kKeepAliveInterval = 0.2;  // 200ms
  const float kBlinkInterval = 0.25;     // 200ms

  auto start_time = HAL_GetTick() / 1000.0f;
  float schedule_1 = start_time + kKeepAliveInterval;
  float schedule_2 = start_time + kKeepAliveInterval;
  float schedule_blink = start_time + kBlinkInterval;

  while (1) {
    //* Task
    robotics::logger::core::LoggerProcess();
    MX_USB_HOST_Process();

    //* Time
    auto current_time = HAL_GetTick() / 1000.0f;

    //* Board LED Blink
    if (schedule_blink < current_time) {
      board_led::Toggle(board_led::kPin1);
      schedule_blink = current_time + kBlinkInterval;
    }

    if (!is_usb_hid_connected) continue;

    //* Update
    vs_ps4::state::entries_1->Update();
    vs_ps4::state::entries_2->Update();
    vs_ps4::state::entries_other->Update();

    auto entry_other = vs_ps4::state::entries_other->FindMostDirtyEntry();
    if (entry_other) {
      entry_other->Invalidate();
    }

    //* Connection scheduler
    auto entry_1 = vs_ps4::state::entries_1->FindMostDirtyEntry();
    if (entry_1) {
      entry_1->Invalidate();
      schedule_1 = current_time + kKeepAliveInterval;
    } else if (schedule_1 < current_time) {
      keep_alive->SendKeepAliveTo(pipe1_remote);
      schedule_1 = current_time + kKeepAliveInterval;
    }

    auto entry_2 = vs_ps4::state::entries_2->FindMostDirtyEntry();
    if (entry_2) {
      entry_2->Invalidate();
      schedule_2 = current_time + kKeepAliveInterval;
    } else if (schedule_2 < current_time) {
      keep_alive->SendKeepAliveTo(pipe2_remote);
      schedule_2 = current_time + kKeepAliveInterval;
    }
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  RCC_OscInitStruct.PLL.PLLR = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);

  /*Configure GPIO pins : PC7 PC8 PC9 */

  GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief  The function is a callback about HID Data events
 *  @param  phost: Selected device
 * @retval None
 */
extern "C" void USBH_HID_EventCallback(USBH_HandleTypeDef *phost) {
  HID_TypeTypeDef hidType;
  HID_KEYBD_Info_TypeDef *keybd_info;
  HID_MOUSE_Info_TypeDef *mouse_info;

  hidType = USBH_HID_GetDeviceType(phost);
  if (hidType != 0xff) {
    printf("HID Device Type: %d\n", hidType);
  }

  auto ptr = USBH_HID_RawGetReport(phost);
  if (ptr[0] != 0x01) {
    printf("ReportID: %08x\n", ptr[0]);
    return;
  }

  if (!is_usb_hid_connected) {
    is_usb_hid_connected = true;
    board_led::On(board_led::kPin2);
  }

  auto stick_left_x = (ptr[1] - 128) / 128.0;
  auto stick_left_y = (128 - ptr[2]) / 128.0;

  auto stick_left = robotics::types::JoyStick2D(stick_left_x, stick_left_y);
  auto old_stick_left_x = vs_ps4::state::stick_left_value;

  if ((stick_left - old_stick_left_x).Magnitude() > 0.01) {
    vs_ps4::state::stick_left_value = stick_left;
  }

  auto stick_right_x = (ptr[3] - 128) / 128.0;
  auto stick_right_y = (128 - ptr[4]) / 128.0;

  auto stick_right = robotics::types::JoyStick2D(stick_right_x, stick_right_y);
  auto old_stick_right_x = vs_ps4::state::stick_right_value;

  if ((stick_right - old_stick_right_x).Magnitude() > 0.01) {
    vs_ps4::state::stick_right_value = stick_right;
  }

  // 0: up |       |      |
  // 1: up | right |      |
  // 2:    | right |      |
  // 3:    | right | down |
  // 4:    |       | down |
  // 5:    |       | down | left
  // 6:    |       |      | left
  // 7: up |       |      | left
  // 8:    |       |      |

  // set
  switch (ptr[5] & 0x0F) {
    case 0:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kUp;
      break;
    case 1:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kUpRight;
      break;
    case 2:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kRight;
      break;
    case 3:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kDownRight;
      break;
    case 4:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kDown;
      break;
    case 5:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kDownLeft;
      break;
    case 6:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kLeft;
      break;
    case 7:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kUpLeft;
      break;
    default:
      vs_ps4::state::dpad_value = nhk2024b::ps4_con::DPad::kNone;
      break;
  }

  vs_ps4::state::button_square_value = ptr[5] & 0x10;
  vs_ps4::state::button_cross_value = ptr[5] & 0x20;
  vs_ps4::state::button_circle_value = ptr[5] & 0x40;
  vs_ps4::state::button_triangle_value = ptr[5] & 0x80;

  vs_ps4::state::button_l1_value = ptr[6] & 0x01;
  vs_ps4::state::button_r1_value = ptr[6] & 0x02;
  vs_ps4::state::button_share_value = ptr[6] & 0x10;
  vs_ps4::state::button_options_value = ptr[6] & 0x20;
  vs_ps4::state::button_l3_value = ptr[6] & 0x40;
  vs_ps4::state::button_r3_value = ptr[6] & 0x80;

  vs_ps4::state::button_ps_value = ptr[7] & 0x01;
  vs_ps4::state::button_touchPad_value = ptr[7] & 0x02;

  vs_ps4::state::trigger_l_value =
      is_controller_stopped ? 0 : (ptr[8] / 255.0f);
  vs_ps4::state::trigger_r_value =
      is_controller_stopped ? 0 : (ptr[9] / 255.0f);
  vs_ps4::state::battery_level_value = ptr[12] / 255.0f;
}

extern "C" int _write(int file, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 10);
  return len;
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
