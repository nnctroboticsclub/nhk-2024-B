# Robobus CAN ID Format

| Character | Bitwidth | Desc                       |
| :-------- | -------- | :------------------------- |
| d/D       | 10b      | Device ID                  |
| p         | 20b      | P2P Pipe ID                |
| s         | 11b      | Session ID                 |
| c         | 1b       | Data/Ctrl Marker           |
| w         | 1b       | Direction(P2H, H2P) Marker |

| Type             |   3   |   8   |   6   |   2   | ::display()     |
| ---------------- | :---: | :---: | :---: | :---: | --------------- |
| Control transfer |   0   |  d1   |   0   |   c   | {d1} <=> mother |
| Raw P2P          |   1   |  d1   |  d2   |       | {d2} ==> {d1}   |
| P2P              |   2   |  p1   |       |   c   | @{p1}           |
| Multicast        |   3   |  s1   |  d1   |       | {d1} ==> #{s1}  |

## Control Transfer

### Control Marker

|    value    | Desc |
| :---------: | :--- |
| kServerData | 0    |
| kServerCtrl | 1    |
| kClientData | 2    |
| kClientCtrl | 3    |
### Control Pipe

| byte | desc     |
| :--- | :------- |
| 0    | TxSeq    |
| 1    | ^        |
| 2    | RxSeq    |
| 3    | ^        |
| 4    | ChunkCRC |
| 5    | ^        |
| 4    | FullCRC  |
| 5    | ^        |

## App
enumerate: find, respond, reset_id, set_id, get_descriptor
rbus: property, cell, method, signal
interface:
  00: enumerate
  01: SDS
  10: Ident

8ac7 2304 89e8 0000