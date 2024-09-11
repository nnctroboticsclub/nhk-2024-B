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

#include <robotics/network/ssp/ssp.hpp>
#include <robotics/network/ssp/value_store.hpp>

#include <nhk2024b/ps4_con.hpp>
#include <robotics/types/joystick_2d.hpp>

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
extern "C" void MX_USB_HOST_Process(void);

namespace robotics::system {
void SleepFor(std::chrono::milliseconds duration) {
  HAL_Delay(duration.count());
}
}  // namespace robotics::system

namespace nhk2024b::controller {

robotics::logger::Logger app_logger{"app", "   App   "};

namespace im920 {
class IM920TxCStream {
  srobo2::ffi::CStreamTx tx;
  bool is_sending = false;

  static void Write(const void *instance, const void *context,
                    const uint8_t *data, size_t len) {
    auto self = const_cast<IM920TxCStream *>(
        reinterpret_cast<const IM920TxCStream *>(instance));
    self->DoWrite(data, len);
  }

  void DoWrite(const uint8_t *data, size_t len) {
    HAL_UART_Transmit(&huart1, const_cast<uint8_t *>(data), len, 1000);

    // app_logger.Info("IM920: Sent %d bytes", len);
    // app_logger.Hex(robotics::logger::core::Level::kInfo, data, len);

    // static char buffer[0x100];
    // for (size_t i = 0; i < len; i++) {
    // buffer[i] = isprint(data[i]) ? data[i] : '.';
    // }
    // buffer[len] = '\0';
    // app_logger.Info("IM920: Sent: %s", buffer);
  }

  static IM920TxCStream instance;

 public:
  static IM920TxCStream *GetInstance() { return &instance; }

  void Init() { srobo2::ffi::__ffi_cstream_associate_tx(&tx, this, &Write); }

  srobo2::ffi::CStreamTx *GetTx() { return &tx; }
};
IM920TxCStream IM920TxCStream::instance;

class IM920RxCStream {
  srobo2::ffi::CStreamRx *rx;
  uint8_t data[1];

  static IM920RxCStream instance;

 public:
  static IM920RxCStream *GetInstance() { return &instance; }

  void Init() {
    rx = srobo2::ffi::__ffi_cstream_new_rx();

    HAL_UART_Receive_IT(&huart1, data, 1);
  }

  void RxIRQ() {
    srobo2::ffi::__ffi_cstream_feed_rx(rx, data, 1);
    HAL_UART_Receive_IT(&huart1, data, 1);

    // app_logger.Info("IM920: Rx: %02x", data[0]);
  }

  srobo2::ffi::CStreamRx *GetRx() { return rx; }
};

IM920RxCStream IM920RxCStream::instance;

srobo2::com::CIM920 *GetIM920() {
  static srobo2::com::CIM920 *im920 = nullptr;
  if (im920 == nullptr) {
    im920 = new srobo2::com::CIM920(
        IM920TxCStream::GetInstance()->GetTx(),
        IM920RxCStream::GetInstance()->GetRx(),
        srobo2::timer::HALCTime::GetInstance()->GetTime());
  }

  return im920;
}

extern "C" void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    IM920RxCStream::GetInstance()->RxIRQ();
  }
}

}  // namespace im920

void Init() { app_logger.Info("Start!"); }

}  // namespace nhk2024b::controller

void ResetIM920() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);  // causes im920's reset
  HAL_Delay(100);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);  // up again
  HAL_Delay(100);
}

namespace vs_ps4 {
robotics::logger::Logger logger{"vs_ps4", "   PS4   "};

bool initialized = false;
robotics::Node<robotics::types::JoyStick2D> *stick_left = nullptr;
robotics::Node<robotics::types::JoyStick2D> *stick_right = nullptr;
robotics::Node<nhk2024b::ps4_con::DPad> *dpad = nullptr;
robotics::Node<bool> *button_square = nullptr;
robotics::Node<bool> *button_cross = nullptr;
robotics::Node<bool> *button_circle = nullptr;
robotics::Node<bool> *button_triangle = nullptr;
robotics::Node<bool> *button_share = nullptr;
robotics::Node<bool> *button_options = nullptr;
robotics::Node<bool> *button_ps = nullptr;
robotics::Node<bool> *button_touchPad = nullptr;
robotics::Node<bool> *button_l1 = nullptr;
robotics::Node<bool> *button_r1 = nullptr;
robotics::Node<bool> *button_l3 = nullptr;
robotics::Node<bool> *button_r3 = nullptr;
robotics::Node<float> *trigger_l = nullptr;
robotics::Node<float> *trigger_r = nullptr;
robotics::Node<float> *battery_level = nullptr;

namespace state {
float stick_left_x = 0;
float stick_left_y = 0;
float stick_right_x = 0;
float stick_right_y = 0;
nhk2024b::ps4_con::DPad dpad_value = nhk2024b::ps4_con::DPad::kNone;
bool button_square_value = false;
bool button_cross_value = false;
bool button_circle_value = false;
bool button_triangle_value = false;
bool button_share_value = false;
bool button_options_value = false;
bool button_ps_value = false;
bool button_touchPad_value = false;
bool button_l1_value = false;
bool button_r1_value = false;
bool button_l3_value = false;
bool button_r3_value = false;
float trigger_l_value = 0;
float trigger_r_value = 0;
float battery_level_value = 0;

void Update() {
  stick_left->SetValue(robotics::types::JoyStick2D{stick_left_x, stick_left_y});
  stick_right->SetValue(
      robotics::types::JoyStick2D{stick_right_x, stick_right_y});
  dpad->SetValue(dpad_value);

  button_square->SetValue(button_square_value);
  button_cross->SetValue(button_cross_value);
  button_circle->SetValue(button_circle_value);
  button_triangle->SetValue(button_triangle_value);
  button_share->SetValue(button_share_value);
  button_options->SetValue(button_options_value);
  button_ps->SetValue(button_ps_value);
  button_touchPad->SetValue(button_touchPad_value);
  button_l1->SetValue(button_l1_value);
  button_r1->SetValue(button_r1_value);
  button_l3->SetValue(button_l3_value);
  button_r3->SetValue(button_r3_value);
  trigger_l->SetValue(trigger_l_value);
  trigger_r->SetValue(trigger_r_value);
  battery_level->SetValue(battery_level_value);
}
}  // namespace state

void RegisterWatcher() {
  stick_left->SetChangeCallback([](robotics::types::JoyStick2D v) {
    logger.Info("[Local] stick_left: %lf %lf", v[0], v[1]);
  });
  stick_right->SetChangeCallback([](robotics::types::JoyStick2D v) {
    logger.Info("[Local] stick_right: %lf %lf", v[0], v[1]);
  });
  dpad->SetChangeCallback(
      [](nhk2024b::ps4_con::DPad v) { logger.Info("[Local] dpad: %d", v); });
  button_square->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_square: %d", v); });
  button_cross->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_cross: %d", v); });
  button_circle->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_circle: %d", v); });
  button_triangle->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_triangle: %d", v); });
  button_share->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_share: %d", v); });
  button_options->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_options: %d", v); });
  button_ps->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_ps: %d", v); });
  button_touchPad->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_touchPad: %d", v); });
  button_l1->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_l1: %d", v); });
  button_r1->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_r1: %d", v); });
  button_l3->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_l3: %d", v); });
  button_r3->SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_r3: %d", v); });
  trigger_l->SetChangeCallback(
      [](float v) { logger.Info("[Local] trigger_l: %lf", v); });
  trigger_r->SetChangeCallback(
      [](float v) { logger.Info("[Local] trigger_r: %lf", v); });
  battery_level->SetChangeCallback(
      [](float v) { logger.Info("[Local] battery_level: %lf", v); });
}

void Init() {
  initialized = true;
  stick_left = new robotics::node::Node<robotics::types::JoyStick2D>();
  stick_right = new robotics::node::Node<robotics::types::JoyStick2D>();
  dpad = new robotics::node::Node<nhk2024b::ps4_con::DPad>();

  button_square = new robotics::node::Node<bool>();
  button_cross = new robotics::node::Node<bool>();
  button_circle = new robotics::node::Node<bool>();
  button_triangle = new robotics::node::Node<bool>();
  button_share = new robotics::node::Node<bool>();
  button_options = new robotics::node::Node<bool>();
  button_ps = new robotics::node::Node<bool>();
  button_touchPad = new robotics::node::Node<bool>();
  button_l1 = new robotics::node::Node<bool>();
  button_r1 = new robotics::node::Node<bool>();
  button_l3 = new robotics::node::Node<bool>();
  button_r3 = new robotics::node::Node<bool>();

  trigger_l = new robotics::node::Node<float>();
  trigger_r = new robotics::node::Node<float>();
  battery_level = new robotics::node::Node<float>();
}

};  // namespace vs_ps4

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

  printf("\n\n\nProgram started\n");

  MX_USB_HOST_Init();
  MX_USART1_UART_Init();

  robotics::logger::core::Init();
  nhk2024b::controller::Init();

  using robotics::network::ssp::SerialServiceProtocol;
  using robotics::network::ssp::ValueStoreService;

  ResetIM920();

  auto im920 =
      srobo2::com::IM910_SRobo1(nhk2024b::controller::im920::GetIM920());

  auto nn = im920.GetNodeNumber();
  printf("NN: 0x%04x (Node Number)\n", nn);
  printf("GN: 0x%08x (Group Number)\n", im920.GetGroupNumber());

  auto remote = nn == 0x0001 ? 0xb732 : 0x0001;
  printf("remote: 0x%04x\n", remote);

  vs_ps4::Init();
  // vs_ps4::RegisterWatcher();

  SerialServiceProtocol<uint16_t> ssp(im920);
  auto vs = ssp.RegisterService<ValueStoreService<uint16_t>>();

  vs->AddController(0, remote, *vs_ps4::stick_left);
  vs->AddController(1, remote, *vs_ps4::stick_right);
  vs->AddController(2, remote, *vs_ps4::dpad);
  vs->AddController(3, remote, *vs_ps4::button_square);
  vs->AddController(4, remote, *vs_ps4::button_cross);
  vs->AddController(5, remote, *vs_ps4::button_circle);
  vs->AddController(6, remote, *vs_ps4::button_triangle);
  vs->AddController(7, remote, *vs_ps4::button_share);
  vs->AddController(8, remote, *vs_ps4::button_options);
  vs->AddController(9, remote, *vs_ps4::button_ps);
  vs->AddController(10, remote, *vs_ps4::button_touchPad);
  vs->AddController(11, remote, *vs_ps4::button_l1);
  vs->AddController(12, remote, *vs_ps4::button_r1);
  vs->AddController(13, remote, *vs_ps4::button_l3);
  vs->AddController(14, remote, *vs_ps4::button_r3);
  vs->AddController(15, remote, *vs_ps4::trigger_l);
  vs->AddController(16, remote, *vs_ps4::trigger_r);
  vs->AddController(17, remote, *vs_ps4::battery_level);

  if (nn != 1) {
    // vs_ps4::stick_left->SetChangeCallback([](robotics::types::JoyStick2D v) {
    //   nhk2024b::controller::app_logger.Info("StickLeft: %f %f", v[0], v[1]);
    // });
    // vs_ps4::stick_right->SetChangeCallback([](robotics::types::JoyStick2D v)
    // {
    //   nhk2024b::controller::app_logger.Info("StickRight: %f %f", v[0], v[1]);
    // });
    vs_ps4::dpad->SetChangeCallback([](nhk2024b::ps4_con::DPad v) {
      nhk2024b::controller::app_logger.Info("DPad: %d", static_cast<int>(v));
    });

    vs_ps4::button_square->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("Square: %d", v); });
    vs_ps4::button_cross->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("Cross: %d", v); });
    vs_ps4::button_circle->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("Circle: %d", v); });
    vs_ps4::button_triangle->SetChangeCallback([](bool v) {
      nhk2024b::controller::app_logger.Info("Triangle: %d", v);
    });
    vs_ps4::button_share->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("Share: %d", v); });
    vs_ps4::button_options->SetChangeCallback([](bool v) {
      nhk2024b::controller::app_logger.Info("Options: %d", v);
    });
    vs_ps4::button_ps->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("PS: %d", v); });
    vs_ps4::button_touchPad->SetChangeCallback([](bool v) {
      nhk2024b::controller::app_logger.Info("TouchPad: %d", v);
    });
    vs_ps4::button_l1->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("L1: %d", v); });
    vs_ps4::button_r1->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("R1: %d", v); });
    vs_ps4::button_l3->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("L3: %d", v); });
    vs_ps4::button_r3->SetChangeCallback(
        [](bool v) { nhk2024b::controller::app_logger.Info("R3: %d", v); });

    vs_ps4::trigger_l->SetChangeCallback([](float v) {
      nhk2024b::controller::app_logger.Info("TriggerL: %f", v);
    });
    vs_ps4::trigger_r->SetChangeCallback([](float v) {
      nhk2024b::controller::app_logger.Info("TriggerR: %f", v);
    });
    // vs_ps4::battery_level->SetChangeCallback([](float v) {
    //   nhk2024b::controller::app_logger.Info("Battery: %f", v);
    // });
  }

  int i = 1;

  HAL_Delay(1000);
  printf("Entering Main loop\n");

  while (1) {
    robotics::logger::core::LoggerProcess();
    MX_USB_HOST_Process();
    if (nn == 1) vs_ps4::state::Update();

    i += 1;
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
static void SystemClock_Config(void) {
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
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /**USART1 GPIO Configuration
  PA9     ------> USART1_TX
  PA10     ------> USART1_RX
  */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 19200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  nhk2024b::controller::im920::IM920RxCStream::GetInstance()->Init();
  nhk2024b::controller::im920::IM920TxCStream::GetInstance()->Init();
  srobo2::timer::HALCTime::GetInstance()->Init();
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
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

  if (!vs_ps4::initialized) return;

  vs_ps4::state::stick_left_x = (ptr[1] - 128) / 128.0;
  vs_ps4::state::stick_left_y = (ptr[2] - 128) / 128.0;
  vs_ps4::state::stick_right_x = (ptr[3] - 128) / 128.0;
  vs_ps4::state::stick_right_y = (ptr[4] - 128) / 128.0;

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

  vs_ps4::state::trigger_l_value = ptr[8] / 255.0f;
  vs_ps4::state::trigger_r_value = ptr[9] / 255.0f;
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
