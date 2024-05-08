from client.client_abc import ClientABC

from textual.widget import Widget
from textual.widgets import RichLog
from textual.app import ComposeResult


class CANRxWidget(Widget):
    def __init__(self, client: ClientABC):
        super().__init__()

        self.can_rx_log = RichLog()
        self.client = client

        self.client.on_can_rx(self.on_can_rx)

    def on_can_rx(self, msg_id, msg_data):
        line = f"{msg_id:03x}: {msg_data.hex()} ({len(msg_data)})"
        self.can_rx_log.write(line)

    def compose(self) -> ComposeResult:
        yield self.can_rx_log
