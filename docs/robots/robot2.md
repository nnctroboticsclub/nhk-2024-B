# Robot 2

## ピン配置
```
SW: PC(0,1,2,3)
CAN1: T PA12; R PA11
CAN2: T PB6; R PB5
FEP: Td  PC6; Rd  PC7; Ini PC8; Rst PC9
ENC: PA8, PA9 (A/B: ?)
EMC: PA15
BNO: SCL PB8; SDA PB9; Rst PB0
LED: PB15 PB14 PC10 PC12 PB7 PB4
```

## OCUI (On-Chip User Interface) 割当

### ブート周り

|LED0|LED1|モード|
|:-:|:-:|:-:|
|0|0|テスト 0|
|0|1|テスト 1|
|1|0|テスト 2|
|1|1|実環境コード|


## コントローラー

|部品|機能|種類|
|:-:|:-:|:-:|
|足|移動|ジョイスティック|
|橋|パージ|ボタン|
|橋|アンロック|ボタン|
|橋|巻き上げ|ボタン|

## ブロック図

```mermaid
graph TD
  F4[F446]
  EMC[EMC]
  ESP32[ESP32]
  c620_0[c620 0]
  c620_1[c620 1]
  Srv[サーボ基板]
  Srv0[ロック周り 0]
  Srv1[ロック周り 1]
  expand[展開]
  F4 --> EMC
  F4 -- CAN1 --> c620_0
  F4 -- CAN1 --> c620_1
  F4 -- SPI --> ESP32
  F4 -- CAN2 --> Srv
  Srv --> Srv0
  Srv --> Srv1
  F4 -- ??? --> expand
```