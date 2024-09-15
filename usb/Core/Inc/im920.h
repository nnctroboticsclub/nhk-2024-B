#pragma once

#include <srobo2/ffi/base.hpp>

#include <logger.h>

namespace nhk2024b::controller::im920 {
UART_HandleTypeDef huart1;

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
}

class IM920TxCStream {
  srobo2::ffi::CStreamTx tx;
  bool is_sending = false;

  static void Write(const void *instance, const void *context,
                    const uint8_t *data, size_t len) {
    auto self = const_cast<IM920TxCStream *>(
        reinterpret_cast<const IM920TxCStream *>(instance));
    self->DoWrite(data, len);
  }

 public:
  void DoWrite(const uint8_t *data, size_t len) {
    HAL_UART_Transmit(&huart1, const_cast<uint8_t *>(data), len, 1000);

    // app_logger.Info("IM920: Sent %d bytes", len);
    // app_logger.Hex(robotics::logger::core::Level::kInfo, data, len);

    // static char buffer[0x100];
    // for (size_t i = 0; i < len; i++) {
    //   buffer[i] = isprint(data[i]) ? data[i] : '.';
    // }
    // buffer[len] = '\0';
    // app_logger.Info("IM920: Sent: %s", buffer);
  }

 private:
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

  void ControlRunning(bool running) {
    if (running) {
      HAL_UART_Receive_IT(&huart1, data, 1);
    } else {
      HAL_UART_AbortReceive(&huart1);
    }
  }

  srobo2::ffi::CStreamRx *GetRx() { return rx; }
};

IM920RxCStream IM920RxCStream::instance;

extern "C" void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    IM920RxCStream::GetInstance()->RxIRQ();
  }
}

static srobo2::com::IM920_SRobo1 *im920 = nullptr;

void ResetIM920() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  printf("Resetting IM920\n");
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);  // causes im920's reset
  HAL_Delay(100);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);  // up again
  HAL_Delay(100);
  printf("Resetted IM920\n");
}

void Init() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  ResetIM920();
  MX_USART1_UART_Init();

  auto rx = nhk2024b::controller::im920::IM920RxCStream::GetInstance();
  rx->Init();

  auto tx = nhk2024b::controller::im920::IM920TxCStream::GetInstance();
  tx->Init();

  auto ctime = srobo2::timer::HALCTime::GetInstance();
  ctime->Init();

  auto cim920 =
      new srobo2::com::CIM920(tx->GetTx(), rx->GetRx(), ctime->GetTime());
  im920 = new srobo2::com::IM920_SRobo1(cim920);

  im920->EnableWrite();
  im920->SetChannel(0x02);

  printf("GN: 0x%08x (Group Number)\n", im920->GetGroupNumber());
  printf("NN: 0x%04x (Node Number)\n", im920->GetNodeNumber());
  printf("Ch: 0x%02x\n", im920->GetChannel());
}

srobo2::com::IM920_SRobo1 *GetIM920() {
  if (im920 == nullptr) {
    Init();
  }

  return im920;
}

}  // namespace nhk2024b::controller::im920