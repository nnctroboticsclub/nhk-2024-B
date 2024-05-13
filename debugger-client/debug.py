from concurrent.futures import ThreadPoolExecutor
import logging
from typing import Callable
import bleak
import asyncio
import struct

import bleak.backends
import bleak.backends.characteristic
import bleak.backends.descriptor

from textual.widget import Widget
from textual.containers import Horizontal, Vertical
from textual.widgets import Tree, Input, RichLog, TabbedContent, TabPane, Label
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


class Client:
    logger = logging.getLogger("CAN Debugger")

    def __init__(self):
        self.ble: bleak.BleakClient | None = None
        self.bus_load = reactive("XXX")
        self.connected = reactive(False)

        self.can_rx_cb: list[Callable[[str, bytearray], None]] = []

        self.keep_alive_task = asyncio.create_task(self.debugger_keep_alive_task())

    async def connect(self, mac):
        if self.ble is not None:
            raise RuntimeError("Already connected")

        self.ble = bleak.BleakClient(mac)

        self.logger.info("Connecting...")
        if self.ble.is_connected:
            self.logger.info("Already connected!")
        else:
            self.logger.info("Not connected, attempting to connect...")
            await self.ble.connect()
        self.logger.info("Connected")
        self.connected = True

        self.logger.debug("Adding notify handlers")
        await self.ble.start_notify(
            "59ef1d41-cb7e-467b-bc7e-5bb7795e1f4c", self.on_bus_load
        )
        await self.ble.start_notify(
            "3eff87b2-613a-4efb-a204-745389e129e8", self.on_can_rx_received
        )
        self.logger.debug("Initializing done")

    def on_bus_load(self, sensor, value: bytearray):
        self.bus_load = f"{value}"

    def on_can_rx_received(self, sensor, value: bytearray):
        msg_id, msg_len, msg_data = CAN_BUS_DATA_FORMAT.unpack(value)
        msg_data = msg_data[:msg_len]
        for cb in self.can_rx_cb:
            cb(msg_id, msg_data)

    def on_can_rx(self, cb: Callable[[str, bytearray], None]):
        self.can_rx_cb.append(cb)

    async def debugger_keep_alive_task(self):
        while True:
            await asyncio.sleep(1)

            if not self.connected:
                continue

            try:
                await self.send_can_packet(0x0542, b"\x55")
            except Exception as e:
                continue

    async def send_can_packet(self, id: int, data: bytes):
        if len(data) > 8:
            raise ValueError("Data length must be <= 8")

        if id > 0x7FF:
            raise ValueError("ID must be <= 0x7FF")

        if self.ble is None:
            raise RuntimeError("Not connected")

        logging.debug(f"Sending CAN packet: {id:03x} ({len(data)}): {data.hex()}")

        packet = CAN_BUS_DATA_FORMAT.pack(id, len(data), data)
        await self.ble.write_gatt_char(
            "18077ff8-d61a-4c04-81da-6217d5739d4e",
            packet,
        )


class BusLoadWidget(Widget):
    def __init__(self, client: Client):
        super().__init__()

        self.client = client

    def render(self) -> str:
        return f"Load: {self.client.bus_load}"


class CANTxWidget(Widget):
    error = reactive("")

    def __init__(self, client: Client):
        super().__init__()

        self.client = client

    def compose(self) -> ComposeResult:
        yield Input(placeholder="CAN message (ID: Data)")
        yield Label(self.error)

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

        try:
            await self.client.send_can_packet(msg_id_int, msg_bytes)
        except Exception as e:
            self.error = f"Failed to send CAN message: {e}"
            return

        self.query_one(Input).value = ""

    async def on_input_submitted(self, event: Input.Submitted) -> None:
        await self.send(event.value)

    def on_input_changed(self, event: Input.Changed) -> None:
        self.error = ""


class CANRxWidget(Widget):
    def __init__(self, client: Client):
        super().__init__()

        self.can_rx_log = RichLog()
        self.client = client

        self.client.on_can_rx(self.on_can_rx)

    def on_can_rx(self, msg_id, msg_data):
        self.can_rx_log.write(f"{msg_id:03x}: {msg_data.hex()} ({len(msg_data)})")

    def compose(self) -> ComposeResult:
        yield self.can_rx_log


class DebuggerApp(App):
    CSS_PATH = "debugger.tcss"

    def __init__(self, client: Client):
        super().__init__()

        self.client = client

    def compose(self) -> ComposeResult:
        with TabbedContent():
            with TabPane("BLE"):
                if self.client.ble is None:
                    yield Label("Not connected")
                else:
                    tree = Tree("Characteristics")
                    for s in self.client.ble.services:
                        service = tree.root.add(
                            f"{s.uuid} ({s.description})", expand=True
                        )
                        for c in s.characteristics:
                            service.add_leaf(f"{c.uuid} ({c.properties})")

                    yield tree
            with TabPane("Debugger"):
                with Vertical():
                    with Horizontal(id="can-topbar"):
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


async def client_test():
    options = get_options()

    client = Client()
    await client.connect(options["mac"])

    await client.send_can_packet(0x0542, b"\x55")

    await asyncio.sleep(1)


async def run_gui():
    options = get_options()

    client = Client()
    try:
        print("Connecting...")
        await client.connect(options["mac"])
        print("Connecting... [ done ]")
        app = DebuggerApp(client)
        await app.run_async()
    except Exception as e:
        print(f"Failed to connect to {options['mac']}: {e}")
        return


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)7s] %(name)-35s: %(message)s",
    )
    asyncio.run(run_gui())
    # asyncio.run(client_test())