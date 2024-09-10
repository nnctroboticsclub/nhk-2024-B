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
    //   buffer[i] = isprint(data[i]) ? data[i] : '.';
    // }
    // buffer[len] = '\0';
    //
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

    app_logger.Info("IM920: Rx: %02x", data[0]);
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

  auto im920 =
      srobo2::com::IM910_SRobo1(nhk2024b::controller::im920::GetIM920());

  auto nn = im920.GetNodeNumber();
  printf("NN: 0x%04x (Node Number)\n", nn);
  printf("GN: 0x%08x (Group Number)\n", im920.GetGroupNumber());

  auto remote = nn == 0x0001 ? 0xb732 : 0x0001;
  printf("remote: 0x%04x\n", remote);

  SerialServiceProtocol<uint16_t> ssp(im920);
  auto vs = ssp.RegisterService<ValueStoreService<uint16_t>>();

  robotics::Node<float> test_node;
  vs->AddController(0, remote, test_node);

  robotics::Node<float> test_node_0002;
  vs->AddController(0, 0x0002, test_node_0002);

  if (nn != 0x0001)
    test_node.SetChangeCallback([](float v) {
      nhk2024b::controller::app_logger.Info("TestNode: %f", v);
    });

  int i = 1;

  while (1) {
    robotics::logger::core::LoggerProcess();
    MX_USB_HOST_Process();

    if (i % 50000 == 0 && nn == 1) {
      // printf("Set %d\n", i);
      test_node.SetValue(i);
      test_node_0002.SetValue(i);
    }

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
    return;
  }

  printf("L2: %lf %lf\n", (ptr[1] - 128) / 128.0, (ptr[2] - 128) / 128.0);
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
