from concurrent.futures import ThreadPoolExecutor
import logging
import asyncio

from textual.widget import Widget

from app import DebuggerApp
from client.ble_client import BLEClient
from client.client_abc import ClientABC
from client.tcp_json_client import TCPJSONClient


async def ainput(prompt: str = "") -> str:
    with ThreadPoolExecutor(1, "AsyncInput") as executor:
        return await asyncio.get_event_loop().run_in_executor(executor, input, prompt)


def get_options():
    import argparse

    parser = argparse.ArgumentParser(description="Debugger Client")
    parser.add_argument("--mac", type=str, help="MAC address of the device")
    parser.add_argument("--server", type=str, help="Server of the TCP JSON Server")
    args = parser.parse_args()

    mac = getattr(args, "mac", None)
    server = getattr(args, "server", None)

    return {"mac": mac, "server": server}


class DeviceStatusesWidget(Widget):
    def __init__(self, client: ClientABC):
        self.client = client

    def render(self) -> str:
        return f"Connected: {self.client.connected}"


async def client_test():
    options = get_options()

    client = BLEClient()
    await client.connect(options["mac"])

    await client.send_can_packet(0x0542, b"\x55")

    await asyncio.sleep(1)


async def run_gui():
    options = get_options()

    client = BLEClient()
    try:
        print("Connecting...")
        await client.connect(options["mac"])
        print("Connecting... [ done ]")
        app = DebuggerApp(client)
        await app.run_async()
    except Exception as e:
        print(f"Failed to connect to {options['mac']}: {e}")
        return


async def run_gui_tj():
    options = get_options()

    client = TCPJSONClient()
    try:
        print("Connecting...")
        await client.connect(options["server"])
        print("Connecting... [ done ]")
        app = DebuggerApp(client)
        await app.run_async()
    except Exception as e:
        print(f"Failed to connect to {options['server']}: {e}")
        return


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)7s] %(name)-35s: %(message)s",
    )
    asyncio.run(run_gui_tj())
    # asyncio.run(client_test())
