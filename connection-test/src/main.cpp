
#include <mbed.h>

#include <sstream>

class FEP_RawDriver {
  mbed::UnbufferedSerial serial_;

  void Send(const std::vector<uint8_t>& data) {
    serial_.write(data.data(), data.size());
  }

  void Send(std::string_view data) { serial_.write(data.data(), data.size()); }

  std::vector<uint8_t> Receive(size_t size) {
    std::vector<uint8_t> data(size);
    serial_.read(data.data(), size);
    return data;
  }

 public:
  FEP_RawDriver(PinName tx, PinName rx, int baud) : serial_(tx, rx, baud) {}

  uint8_t GetRegister(uint8_t address) {
    std::stringstream ss;
    ss << "@REG" << address;
    Send(ss.str());
    return receive(1)[0];
  }
};

int main() { return 0; }
