from client.client_abc import ClientABC

from textual.widget import Widget


class BusLoadWidget(Widget):
    def __init__(self, client: ClientABC):
        super().__init__()

        self.client = client

    def render(self) -> str:
        return f"Load: {self.client.bus_load}"
