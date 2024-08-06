import logging
from typing import Callable
from textual.reactive import reactive
from textual.app import ComposeResult
from textual.widgets import Label


class ClientABC:
    logger = logging.getLogger("ABCClient")

    def __init__(self):
        self.can_rx_cb: list[Callable[[int, bytearray], None]] = []

        self.bus_load = reactive("XXX")
        self.connected = reactive(False)

    def abc__dispatch_can_rx(self, msg_id: int, msg_data: bytearray):
        for cb in self.can_rx_cb:
            cb(msg_id, msg_data)

    def on_can_rx(self, cb: Callable[[int, bytearray], None]):
        self.can_rx_cb.append(cb)

    async def send_can_packet(self, id: int, data: bytes):
        raise NotImplementedError

    def compose_connection(self) -> ComposeResult:
        yield Label(f"compose_connection#{type(self)} is not impl")
