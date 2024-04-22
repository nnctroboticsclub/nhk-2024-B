import bleak
import asyncio

import bleak.backends
import bleak.backends.characteristic
import bleak.backends.descriptor


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
    print(f"CAN RX: {value}")


async def keep_alive(client: bleak.BleakClient):
    while True:
        print("Sending keep-alive...")
        await client.write_gatt_char(
            "18077ff8-d61a-4c04-81da-6217d5739d4e", bytearray([0x42, 0x05, 0x01, 0x55])
        )
        await asyncio.sleep(1)


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

        print("Starting keep-alive task")
        await asyncio.gather(keep_alive(device))

    except Exception as e:
        print(f"Failed to connect to {options['mac']}: {e}")
        return


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run())
