import asyncio
import json
import logging
import socket

from .client_abc import ClientABC

from textual.app import ComposeResult
from textual.widget import Widget
from textual.widgets import Label, Static
from textual.reactive import reactive

from pydantic import BaseModel


class Message(BaseModel):
    service: str
    method: str
    data: dict


class Statistics(Widget):
    stat: reactive = reactive("---", recompose=True)

    def __init__(self, name: str, **kwargs):
        super().__init__(**kwargs)
        self.stat_name = name

    def render(self) -> str:
        return f"{self.stat_name}: {self.stat}"

    def update(self, value: str):
        self.stat = value


class TJStatistics(Widget):
    def __init__(self):
        super().__init__()

        self.statistics = {
            "is_connected": False,
            "read_fail": 0,
            "rx_total": 0,
            "rx_processed": 0,
            "rx_ignored": 0,
            "tx_total": 0,
        }

    def compose(self) -> ComposeResult:
        yield Static("Connection Statistics", id="title")
        yield Statistics("Connected    ", id="is_connected")
        yield Statistics("Read Failed  ", id="read_fail")
        yield Statistics("Rx Total     ", id="rx_total")
        yield Statistics("Rx Processed ", id="rx_processed")
        yield Statistics("Rx Ignored   ", id="rx_ignored")
        yield Statistics("Tx Total     ", id="tx_total")

    def update(self, node_id: str, value: str):
        statistics = self.query("Statistics").results(Statistics)

        for stat in statistics:
            if stat.id == node_id:
                stat.update(value)
                break

    def update_is_connected(self, value: bool):
        status = "Yes" if value else "No"
        self.update("is_connected", status)

    def update_read_fail(self):
        self.statistics["read_fail"] += 1
        self.update("read_fail", f"{self.statistics['read_fail']}")

    def update_rx_total(self):
        self.statistics["rx_total"] += 1
        self.update("rx_total", f"{self.statistics['rx_total']}")

    def update_rx_processed(self):
        self.statistics["rx_processed"] += 1
        self.update("rx_processed", f"{self.statistics['rx_processed']}")

    def update_rx_ignored(self):
        self.statistics["rx_ignored"] += 1
        self.update("rx_ignored", f"{self.statistics['rx_ignored']}")

    def update_tx_total(self):
        self.statistics["tx_total"] += 1
        self.update("tx_total", f"{self.statistics['tx_total']}")


class TCPJSONClient(ClientABC):
    logger = logging.getLogger("TJClient")

    def __init__(self):
        super().__init__()

        self.reader: asyncio.StreamReader | None = None
        self.writer: asyncio.StreamWriter | None = None

        self.keep_alive_task = asyncio.create_task(self.debugger_keep_alive_task())
        self.server_task_ = asyncio.create_task(self.server_task())

        self.statistics_widget = TJStatistics()

    async def connect(self, server: str | None):
        if server is None:
            raise RuntimeError("Server is required")

        if not server.find(":"):
            server += ":8900"

        host, port = server.split(":", 1)

        if any(x not in "0123456789" for x in port):
            print(repr(port))
            raise ValueError("Port format is must be decimal")

        port = int(port)

        self.logger.info("Connecting...")
        (self.reader, self.writer) = await asyncio.open_connection(host, port)

        self.logger.info("Connected")
        self.connected = True
        self.statistics_widget.update_is_connected(True)

        self.logger.debug("Initializing done")

    def on_bus_load(self, sensor, value: bytearray):
        self.bus_load = f"{value}"

    async def server_task(self):
        while True:
            if not bool(self.connected) or not self.reader:
                self.statistics_widget.update_read_fail()
                await asyncio.sleep(1)
                continue

            line = await self.reader.readline()
            if not line:
                self.logger.warn("Failed to read line")
                self.connected = False
                self.statistics_widget.update_is_connected(False)

                if self.writer:
                    self.writer.close()
                self.writer = None

                self.reader = None
                continue

            raw_json = line.decode()
            self.statistics_widget.update_rx_total()

            try:
                obj = json.loads(raw_json)
            except json.JSONDecodeError:
                self.logger.warn("Failed to decode json")
                continue

            try:
                message = Message(**obj)
            except Exception as e:
                self.logger.warn(f"Failed to parse message: {e}")
                continue

            if message.service != "CAN Debugger":
                self.statistics_widget.update_rx_ignored()
                continue

            self.statistics_widget.update_rx_processed()

            if message.method == "can_rx":
                msg_id = message.data["id"]
                payload = message.data["payload"]

                byte_payload = bytearray(payload)

                self.abc__dispatch_can_rx(msg_id, byte_payload)

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

        if self.writer is None:
            raise RuntimeError("Not connected")

        logging.debug(f"Sending CAN packet: {id:03x} ({len(data)}): {data.hex()}")

        # ...

    def compose_connection(self) -> ComposeResult:
        yield self.statistics_widget
