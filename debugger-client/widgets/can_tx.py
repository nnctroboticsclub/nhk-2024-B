from client.client_abc import ClientABC

from textual.widget import Widget
from textual.reactive import reactive
from textual.widgets import Input, Label
from textual.app import ComposeResult


class CANTxWidget(Widget):
    error = reactive("")

    def __init__(self, client: ClientABC):
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
