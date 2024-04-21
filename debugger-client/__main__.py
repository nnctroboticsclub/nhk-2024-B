import bleak
import asyncio


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


async def run():
    options = get_options()

    """ devices = await bleak.BleakScanner.discover()
    for d in devices:
        print(f"{d.name}: {d.address}")
        device = bleak.BleakClient(d)
        try:
            await device.connect()
        except Exception as e:
            print(f"  - XXX: Failed to connect to {d.name}: {e}")
            continue

        for s in device.services:
            print(f"  - {s}")

        await device.disconnect() """

    device = bleak.BleakClient(options["mac"])
    try:
        await device.connect()
        for s in device.services:
            print(f"  - {s}")
            for c in s.characteristics:
                print(f"    - {c} ({c.properties})")
        await device.disconnect()
    except Exception as e:
        print(f"Failed to connect to {options['mac']}: {e}")
        return


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run())
