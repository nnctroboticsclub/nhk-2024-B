# Note

## Value Store ID 割当

value store ID 割当

例: 24000000
24[年度] 000001[インデックス]

種別
0: NHK ロボコン
それ以外: 予備

インデックス
複数の[コントローラー - 受信機]（パイプ）が存在するときの識別番号
連続である必要はない

## ST-Link のシリアル番号リスト

|          Serial          | Short-Description |
| :----------------------: | :---------------: |
| 066DFF303435554157121020 |    Controller     |
| 066FFF555054877567044942 |   Test Device1    |

## 制御進捗報告
### ロボ 1 @timniku
- [x] 大体 OK
- [x] CAN テスト
- [x] 統合レイヤ
  - [x] モーター割当の修正
- [x] ブレーキ
- [x] 回収機構逆転
- [x] アンロックを角度制御に
- [x] 旋回 (トリガー)
### ロボ 2
- [x] ロボマス/can_servo が全くつながらない
- [x] サーボのいい感じの値をさがす [角度] @timniku (まかせた)
- [x] ロボマス治す (月曜日とか)
### ロボ 3
- [x] 完成
- [x] プロポ対応
### 通信
- [x] IM920(sL) 受信/構成情報読み取り
- [x] IM920(sL) 送信
- [x] IM920(sL) 共通レイヤに乗せる
  - [x] C++ bind
  - [ ] syoch-robotics への組み込み
### DS4 (STM32 MX OTG)
- [x] キーボードでのテスト ok
- [x] 生 HID レポート読み読み
- [x] connection/im920 をマージする
- [x] HAL 用に IM920 読むやつの互換性レイヤを実装
- [x] DS4 コンポーネントと IM920 コンポーネントとを統合 (完成)
- [x] 送り分け
  - [x] 共通ライブラリへ組み込み
  - [x] KeepAlive Service の実装

## Robo3

### pinmap 旧

| M | Fin  | Rin  |
|:-:|:----:|:----:|
| 1 | pa9  | pa8  |
| 2 | pa11 | pa10 |
| 3 | pb9  | pb8  |
| 4 | pb10 | pb2  |

### pinmap 新

```cpp
DigitalOut emc{PB_9};

Actuators actuators{(Actuators::Config){
  // M4
  .move_motor_l_fin = PB_4,
  .move_motor_l_rin = PB_3,

  // M3
  .move_motor_r_fin = PB_6,
  .move_motor_r_rin = PB_5,

  // M2
  .arm_elevation_motor_fin = PC_12,
  .arm_elevation_motor_rin = PC_11,

  // M1
  .arm_extension_motor_fin = PC_10,
  .arm_extension_motor_rin = PA_15,
}};
```