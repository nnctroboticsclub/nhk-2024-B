from concurrent.futures import ThreadPoolExecutor
import threading
import bleak
import asyncio
import struct

import bleak.backends
import bleak.backends.characteristic
import bleak.backends.descriptor

from textual.events import Callback
from textual.widget import Widget
from textual.containers import Container, Horizontal, VerticalScroll, Vertical
from textual.widgets import Tree, Input, RichLog, TabbedContent, TabPane, Markdown
from textual.reactive import reactive
from textual.app import App, ComposeResult

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


class BusLoadWidget(Widget):
    def __init__(self, client: bleak.BleakClient):
        super().__init__()

        self.text = reactive("XXX")
        self.client = client

    async def on_mount(self):
        await self.client.start_notify(
            "59ef1d41-cb7e-467b-bc7e-5bb7795e1f4c", self.bus_load
        )

    def bus_load(self, sensor, value: bytearray):
        self.text = f"{value}"

    def render(self) -> str:
        return f"BUs Load: {self.text}"


class CANTxWidget(Widget):
    error = reactive("")

    def __init__(self, client: bleak.BleakClient):
        super().__init__()

        self.client = client

    def compose(self) -> ComposeResult:
        yield Input(placeholder="CAN message (ID: Data)")

    async def send(self, text: str):
        # Parse input
        msg_id, msg_data = text.replace(" ", "").lower().split(":", 1)

        # Validate input
        if len(msg_id) > 3:
            self.error = "Invalid ID Length"

        if len(msg_data) > 16:
            self.error = "Invalid data Length"

        if not all(c in "0123456789abcdef" for c in msg_id):
            self.error = "Invalid ID"

        if not all(c in "0123456789abcdef" for c in msg_data):
            self.error = "Invalid data"

        if len(msg_data) % 2 != 0:
            self.error = "Data must be a multiple of 2"

        # Construct packet
        msg_id_int = int(msg_id, 16)
        msg_bytes = bytearray.fromhex(msg_data)
        packet = bytearray(
            CAN_BUS_DATA_FORMAT.pack(msg_id_int, len(msg_bytes), msg_bytes)
        )

        # Send packet
        await self.client.write_gatt_char(
            "18077ff8-d61a-4c04-81da-6217d5739d4e",
            packet,
        )

        self.query_one(Input).value = ""

    async def on_input_submitted(self, event: Input.Submitted) -> None:
        await self.send(event.value)

    def on_input_changed(self, event: Input.Changed) -> None:
        self.error = ""


class CANRxWidget(Widget):
    def __init__(self, client: bleak.BleakClient):
        super().__init__()

        self.can_rx_log = RichLog()
        self.client = client

    async def on_mount(self):
        await self.client.start_notify(
            "3eff87b2-613a-4efb-a204-745389e129e8", self.can_rx
        )

    def can_rx(self, sensor, value: bytearray):
        msg_id, msg_len, msg_data = CAN_BUS_DATA_FORMAT.unpack(value)
        msg_data = msg_data[:msg_len]
        self.can_rx_log.write(f"{msg_id:03x}: {msg_data.hex()} ({msg_len})")

    def compose(self) -> ComposeResult:
        yield self.can_rx_log


class DebuggerKeepAliveWidget(Widget):
    counter = reactive(0)

    def __init__(self, client: bleak.BleakClient):
        super().__init__()

        self.client = client

    async def task(self):
        while True:
            try:
                await self.client.write_gatt_char(
                    "18077ff8-d61a-4c04-81da-6217d5739d4e",
                    bytearray(struct.pack("<hb8s", 0x0542, 0x01, b"\x55")),
                )
            except Exception as e:
                print(f"Failed to send keep-alive: {e}")
                self.counter = -1
                return
            self.counter += 1
            await asyncio.sleep(1)

    async def on_mount(self):
        asyncio.create_task(self.task())

    def render(self) -> str:
        return f"Sent {self.counter} keep-alive messages"


class DebuggerApp(App):
    CSS_PATH = "debugger.tcss"

    def __init__(self, client: bleak.BleakClient):
        super().__init__()

        self.client = client

    def compose(self) -> ComposeResult:
        with TabbedContent():
            with TabPane("BLE"):
                tree = Tree("Characteristics")
                for s in self.client.services:
                    service = tree.root.add(f"{s.uuid} ({s.description})", expand=True)
                    for c in s.characteristics:
                        service.add_leaf(f"{c.uuid} ({c.properties})")

                yield tree
            with TabPane("Debugger"):
                with Vertical():
                    with Horizontal(id="can-topbar"):
                        yield DebuggerKeepAliveWidget(self.client)
                        yield BusLoadWidget(self.client)
                    yield CANTxWidget(self.client)
                    yield CANRxWidget(self.client)


class CANDebugger:
    def can_rx(self, sensor, value: bytearray):
        msg_id, msg_len, msg_data = CAN_BUS_DATA_FORMAT.unpack(value)
        msg_data = msg_data[:msg_len]
        print(f"CAN RX: {msg_id:03x} ({msg_len}): {msg_data.hex()}")

    async def keep_alive(self, client: bleak.BleakClient):
        while True:
            print("Sending keep-alive...")
            await client.write_gatt_char(
                "18077ff8-d61a-4c04-81da-6217d5739d4e",
                bytearray(struct.pack("<hb8s", 0x0542, 0x01, b"\x55")),
            )
            await asyncio.sleep(1)

    async def debugger_keep_alive(self, client: bleak.BleakClient):
        while True:
            print("Dka")
            await client.write_gatt_char(
                "39bb2e82-5f67-430b-827e-565d67ce7739",
                b"\0\0\0\0",
            )
            await asyncio.sleep(0.2)

    async def stdin_can_relay(self, client: bleak.BleakClient):
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


async def run_gui():
    options = get_options()

    device = bleak.BleakClient(options["mac"])
    try:
        await device.connect()
        app = DebuggerApp(device)
        await app.run_async()
    except Exception as e:
        print(f"Failed to connect to {options['mac']}: {e}")
        return


if __name__ == "__main__":
    asyncio.run(run_gui())
