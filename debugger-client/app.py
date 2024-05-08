from client.client_abc import ClientABC

from textual.app import App, ComposeResult
from textual.widgets import Label, TabbedContent, TabPane
from textual.containers import Vertical, Horizontal

from widgets.bus_load import BusLoadWidget
from widgets.can_rx import CANRxWidget
from widgets.can_tx import CANTxWidget


class DebuggerApp(App):
    CSS_PATH = "debugger.tcss"

    def __init__(self, client: ClientABC):
        super().__init__()

        self.client = client

    def compose(self) -> ComposeResult:
        with TabbedContent():
            with TabPane("BLE"):
                if not self.client.connected:
                    yield Label("Not connected")
                else:
                    yield from self.client.compose_connection()
            with TabPane("Debugger"):
                with Vertical():
                    with Horizontal(id="can-topbar"):
                        yield BusLoadWidget(self.client)
                    yield CANTxWidget(self.client)
                    yield CANRxWidget(self.client)
