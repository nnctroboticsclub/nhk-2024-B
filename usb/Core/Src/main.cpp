/* USER CODE BEGIN Header */
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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbh_hid.h"
#include <stdio.h>
#include <srobo2/ffi/base.hpp>
#include <srobo2/com/im920.hpp>
#include <chrono>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
extern "C" void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

namespace robotics::system {
void SleepFor(std::chrono::milliseconds duration) {
  HAL_Delay(duration.count());
}
}  // namespace robotics::system

namespace nhk2024b::controller {

namespace im920 {
robotics::logger::Logger im920_logger("im920.app", "Com:IM920");

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
    im920_logger.Info("IM920 <-- ");
    im920_logger.Hex(robotics::logger::core::Level::kInfo, data, len);

    HAL_UART_Transmit(&huart1, const_cast<uint8_t *>(data), len, 1000);
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
  }

  srobo2::ffi::CStreamRx *GetRx() { return rx; }
};

IM920RxCStream IM920RxCStream::instance;

class HALCTime {
  srobo2::ffi::CTime time;

  static float Now(const void *context) { return HAL_GetTick() / 1000.0f; }

  static void Sleep(const void *context, float duration) {
    HAL_Delay(duration * 1000);
  }

  static HALCTime instance;

 public:
  static HALCTime *GetInstance() { return &instance; }

  void Init() {
    srobo2::ffi::__ffi_ctime_set_context(&time, this);
    srobo2::ffi::__ffi_ctime_set_now(&time, &Now);
    srobo2::ffi::__ffi_ctime_set_sleep(&time, &Sleep);
  }

  srobo2::ffi::CTime *GetTime() { return &time; }
};

HALCTime HALCTime::instance;

srobo2::com::CIM920 *GetIM920() {
  static srobo2::com::CIM920 *im920 = nullptr;
  if (im920 == nullptr) {
    im920 = new srobo2::com::CIM920(IM920TxCStream::GetInstance()->GetTx(),
                                    IM920RxCStream::GetInstance()->GetRx(),
                                    HALCTime::GetInstance()->GetTime());
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

robotics::logger::Logger app_logger{"app", "   App   "};

void OnIM920Packet(uint16_t from, uint8_t *data, size_t len) {
  app_logger.Info("Received from %d: ", from);
  app_logger.Hex(robotics::logger::core::Level::kInfo, data, len);
}

void Init() {
  auto im920_ = im920::GetIM920();

  auto node_id = im920_->GetNodeNumber(1.0f);
  app_logger.Info("Node ID: %d", node_id);

  auto version = im920_->GetVersion(1.0f);
  app_logger.Info("Version: %s", version.c_str());

  im920_->OnData(&OnIM920Packet);

  im920_->Send(3 - node_id, (const uint8_t *)"Hello, world!", 13, 1.0f);

  app_logger.Info("Start!");
}

}  // namespace nhk2024b::controller
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
extern "C" int main(void) {
  /* USER CODE BEGIN 1 */
  setbuf(stdout, NULL);
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  robotics::logger::core::Init();

  MX_USB_HOST_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  printf("\n\n\nProgram started\n");

  nhk2024b::controller::Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    robotics::logger::core::LoggerProcess();
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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
  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
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
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {
  /* USER CODE BEGIN USART1_Init 0 */
  // nothing
  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */
  printf("HAL_UART_MspInit (UART1)\n");
  /* Peripheral clock enable */
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
  /* USER CODE END USART1_Init 1 */

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
  /* USER CODE BEGIN USART1_Init 2 */

  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  nhk2024b::controller::im920::IM920RxCStream::GetInstance()->Init();
  nhk2024b::controller::im920::IM920TxCStream::GetInstance()->Init();

  /* USER CODE END USART1_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

extern "C" void Test_syoch_01(USBH_HandleTypeDef *phost);

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

  Test_syoch_01(phost);
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
