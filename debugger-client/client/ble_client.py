import struct
from .client_abc import ClientABC

import logging
import asyncio

import bleak
from textual.app import ComposeResult
from textual.widgets import Label, Tree


CAN_BUS_DATA_FORMAT = struct.Struct("<hb8s")


class BLEClient(ClientABC):
    logger = logging.getLogger("BLEClient")

    def __init__(self):
        self.ble: bleak.BleakClient | None = None

        self.keep_alive_task = asyncio.create_task(self.debugger_keep_alive_task())

    async def connect(self, mac):
        if mac is None:
            raise ValueError("MAC Address is required")

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
        self.abc__dispatch_can_rx(msg_id, msg_data)

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

    def compose_connection(self) -> ComposeResult:
        if not self.ble:
            yield Label("!!!BLE")
            return

        tree = Tree("Characteristics")
        for s in self.ble.services:
            service = tree.root.add(f"{s.uuid} ({s.description})", expand=True)
        for c in s.characteristics:
            service.add_leaf(f"{c.uuid} ({c.properties})")

        yield tree
