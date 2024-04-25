# Network

## Graph

```mermaid
graph LR
  e-ctrl[コントローラー送り: ESP32]
  r1-e[コントローラー受け: ESP32]
  r1-se1[メインマイコン: STM32 + ESP32]
  r1-dbg[デバッグ: ESP32]

  プロコン -- Bluetooth --> PC
  PC --UART--> e-ctrl
  自作コントローラー --UART--> e-ctrl

  e-ctrl <-- Bluetooth --> r1-e
  e-ctrl <-- FEP --> r1-e

  r1-e <-- CAN --> r1-se1
  r1-se1 <-- CAN --> r1-dbg
```

## CAN Bus

## Message ID

| s | 0 | 1 | 2 | 3 | 4 | 5 | 6 |  7  | 8 | 9 | 10 | Description         |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:---:|:-:|:-:|:--:| ------------------- |
| - | 0 | x |   |   |   |   |   |     |   |   |    | ikarashiCAN         |
| - | 0 | 0 | 1 | 1 | 1 | 1 | 1 |  1  | 1 | 1 |  1 | C620                |
| ^ | 0 | 1 | 0 | 0 | 0 | 0 | 0 |  0  | 0 | 0 |  0 | ^                   |
|   | 1 | 0 | 0 | 0 | 0 | n |   | Dev |   |   |    | Config for (Dev)[n] |
|   | 1 | 0 | 0 | 0 | 1 | n |   | Dev |   |   |    | Config Applied      |
|   | 1 | 0 | 0 | 1 | 0 | n |   | Dev |   |   |    | Config Request      |
|   | 1 | 0 | 0 | 1 | 1 | 0 | 0 | Dev |   |   |    | Reset Notify        |
|   | 1 | 0 | 0 | 1 | 1 | 0 | 1 | Dev |   |   |    | Node Inspector      |
|   | 1 | 0 | 0 | 1 | 1 | 1 | 0 | Dev |   |   |    | N/A                 |
|   | 1 | 0 | 0 | 1 | 1 | 1 | 1 | Dev |   |   |    | N/A                 |
|   | 1 | 0 | 1 | 0 | 0 | 0 | 0 | Dev |   |   |    | Value store[Dev]    |
|   | 1 | 0 | 1 | 0 | 0 | 0 | 1 | Dev |   |   |    | Report[Dev]         |
| x | 1 | 0 | 1 | 0 | 0 | 1 | 0 | Dev |   |   |    | Pong from [Dev]     |
| x | 1 | 0 | 1 | 0 | 0 | 1 | 1 | Dev |   |   |    | Ping                |
|   | 1 | 0 | 1 | 0 | 1 | 0 | 0 |  0  | 0 | 0 |  0 | Keep Alive (IC)     |
|   | 1 | 0 | 1 | 0 | 1 | 0 | 0 |  0  | 0 | 0 |  1 | Keep Alive (CTRL)   |
|   | 1 | 0 | 1 | 0 | 1 | 0 | 0 |  0  | 0 | 1 |  0 | Keep Alive (Debug)  |
|   | 1 | 0 | 1 | 0 | 1 | 0 | 0 |  0  | 0 | 1 |  1 | N/A                 |
|   | 1 | 0 | 1 | 0 | 1 | 0 | 0 |  0  | 1 | x |  x | N/A                 |
|   | 1 | 0 | 1 | 0 | 1 | 0 | 0 |  1  | x | x |  x | N/A                 |
|   | 1 | 0 | 1 | 0 | 1 | 0 | 1 |  x  | x | x |  x | N/A                 |
|   | 1 | 0 | 1 | 0 | 1 | 1 | x |  x  | x | x |  x | N/A                 |
|   | 1 | 1 | 1 | x |   |   |   |     |   |   |    | Stream              |

### NodeInspector

#### Payload

|   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Type  |       |  Id   |       | Data  |       |       |       |

#### Type

|   ID | Type   |
| ---: | :----- |
| 0000 | int    |
| 0001 | float  |
| 0001 | double |
| 0002 | bool   |

#### Data

|        |   0   |   1   |   2   |   3   |
| -----: | :---: | :---: | :---: | :---: |
|    int |  MSB  |       |  LSB  |       |
|  float |  MSB  |       |       |       |
| double |  MSB  |       |       |       |
|   bool |   0   |   0   |   0   |  0/1  |



### Report
#### Data Types

- MDC4
  - Motors
  - Encoders (Angle)
  - Encoders (RPM)

#### General Data Format

|   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
|  ID   |   >   |   >   |   >   |   >   |   >   |   >   | DATA  |

#### ID types

|   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   | Description           |   0   |   1   |   2   |   3   |   4    |   5   |   6   |   7   |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | --------------------- | :---: | :---: | :---: | :---: | :----: | :---: | :---: | :---: |
|  ID   |       |       |       |       |       |       |       | ^                     |       |       |       |       |        |       |       |       |
|   0   |   0   |   0   |   0   |   0   |  ID   |       |       | MDC[ID] Speeds        |  ID   |   >   |   >   |   >   | Speeds |   >   |   >   |   -   |
|   0   |   0   |   0   |   0   |   1   |  ID   |       |       | MDC[ID] Angles        |   ^   |   >   |   >   |   >   | Angles |   >   |   >   |   -   |
|   0   |   0   |   0   |   1   |   0   |  ID   |       |       | MDC[ID] RPMs          |   ^   |   >   |   >   |   >   |  RPMs  |   >   |   >   |   -   |
|   0   |   0   |   0   |   1   |   1   |  ID   |       |       | MDC[ID] Currents      |   ^   |   >   |   >   |   >   | Currs  |   >   |   >   |   -   |
|   0   |   0   |   1   |   0   |   0   |   0   |   0   |   0   | Pong notify           |   ^   |   i   |   >   |   >   |   >    |   >   |   >   |   -   |
|   0   |   0   |   1   |   0   |   0   |   0   |   0   |   1   | Status update         |   ^   |   i   |   >   | stat  |   >    |   >   |   >   |   -   |
|   0   |   0   |   1   |   0   |   0   |   0   |   1   |   x   | N/A                   |   ^   |   >   |   >   |   >   |   >    |   >   |   >   |   -   |
|   0   |   0   |   1   |   0   |   0   |   1   |   x   |   x   | N/A                   |   ^   |   >   |   >   |   >   |   >    |   >   |   >   |   -   |
|   0   |   1   |   0   |  ID   |       |       |       |       | PID Gain/FB/Out/Error |   ^   |   P   |   I   |   D   |   Fb   |  Ou   |  Gl   |   E   |
|   0   |   1   |   x   |       |       |       |       |       | N/A                   |   ^   |   >   |   >   |   >   |   >    |   >   |   >   |   -   |
|   1   |   0   |   x   |       |       |       |       |       | N/A                   |   ^   |   >   |   >   |   >   |   >    |   >   |   >   |   -   |
|   1   |   1   |   x   |       |       |       |       |       | N/A                   |   ^   |   >   |   >   |   >   |   >    |   >   |   >   |   -   |


## Bluetooth sequence

```mermaid
sequenceDiagram
  participant BLE as BT Stack
  participant GAT as GATT
  participant SRV as Server
  participant SVC as Service
  participant Chr as Characteristic
  participant Att as Attribute

  note over GAT: Initialization
  GAT->>SVC: Add Service
  SVC->>Chr: Add Characteristic
  Chr->>Att: Add Attribute

  note over BLE: Register
  BLE->>GAT: Register
  GAT->>SVC: Register Service with GATTs IF
  SVC->>BLE: Register Attribute table
  BLE->>GAT: Notify Handle IDs
  GAT->>SVC: ;
  SVC->>Att: ;

  note over BLE, Att: Connection
  BLE->> GAT: Connect
  GAT->>+SRV: ;
  SRV->>-GAT: Collect

  note over BLE, Att: Write
  BLE->>GAT: Write
  GAT->>SRV: Write
  SRV->>SVC: Write with switching

  note over BLE, Att: Notify
  SVC->>GAT: Changed
  GAT->>SRV: ;
  SRV->>BLE: Notify

```