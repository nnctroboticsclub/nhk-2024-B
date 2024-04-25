from concurrent.futures import ThreadPoolExecutor
import bleak
import asyncio
import struct

import bleak.backends
import bleak.backends.characteristic
import bleak.backends.descriptor

CAN_BUS_DATA_FORMAT = struct.Struct("<hb8s")


async def ainput(prompt: str = "") -> str:
    with ThreadPoolExecutor(1, "AsyncInput") as executor:
        return await asyncio.get_event_loop().run_in_executor(executor, input, prompt)


def get_options():
    import argparse

    parser = argparse.ArgumentParser(description="Debugger Client")
    parser.add_argument("--mac", type=str, help="MAC address of the device")
    args = parser.parse_args()

    mac = getattr(args, "mac", None)
    if mac is None:
        raise ValueError("MAC address is required")

    return {
        "mac": mac,
    }


def bus_load(sensor, value: bytearray):
    print(f"Bus load: {value}")


def can_rx(sensor, value: bytearray):
    msg_id, msg_len, msg_data = CAN_BUS_DATA_FORMAT.unpack(value)
    msg_data = msg_data[:msg_len]
    print(f"CAN RX: {msg_id:03x} ({msg_len}): {msg_data.hex()}")


async def keep_alive(client: bleak.BleakClient):
    while True:
        print("Sending keep-alive...")
        await client.write_gatt_char(
            "18077ff8-d61a-4c04-81da-6217d5739d4e",
            bytearray(struct.pack("<hb8s", 0x0542, 0x01, b"\x55")),
        )
        await asyncio.sleep(1)


async def stdin_can_relay(client: bleak.BleakClient):
    while True:
        line = await ainput("Enter CAN message (ID: Data): ")
        if not line:
            continue

        # Parse input
        msg_id, msg_data = line.replace(" ", "").lower().split(":", 1)

        # Validate input
        if len(msg_id) > 3:
            print("Invalid ID Length")
            continue

        if len(msg_data) > 16:
            print("Invalid data Length")
            continue

        if not all(c in "0123456789abcdef" for c in msg_id):
            print("Invalid ID")
            continue

        if not all(c in "0123456789abcdef" for c in msg_data):
            print("Invalid data")
            continue

        if len(msg_data) % 2 != 0:
            print("Data must be a multiple of 2")
            continue

        # Construct packet
        msg_id_int = int(msg_id, 16)
        msg_bytes = bytearray.fromhex(msg_data)
        packet = bytearray(
            CAN_BUS_DATA_FORMAT.pack(msg_id_int, len(msg_bytes), msg_bytes)
        )

        # Send packet
        await client.write_gatt_char(
            "18077ff8-d61a-4c04-81da-6217d5739d4e",
            packet,
        )


async def run():
    options = get_options()

    device = bleak.BleakClient(options["mac"])
    try:
        await device.connect()
        for s in device.services:
            print(f"  - {s}")
            for c in s.characteristics:
                print(f"    - {c} ({c.properties})")

        print("Starting notify tasks")
        await device.start_notify("59ef1d41-cb7e-467b-bc7e-5bb7795e1f4c", bus_load)
        await device.start_notify("3eff87b2-613a-4efb-a204-745389e129e8", can_rx)

        # print("Starting keep-alive task")
        # await asyncio.gather(keep_alive(device))

        print("Starting stdin -> CAN task")
        await asyncio.gather(stdin_can_relay(device))

    except Exception as e:
        print(f"Failed to connect to {options['mac']}: {e}")
        return


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run())
