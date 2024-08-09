#pragma once

#include <ikarashiCAN_mk2.h>
#include <can_servo.h>

namespace nhk2024b::common {
class CanServo;

class CanServoBus {
  ikarashiCAN_mk2 &can;
  can_servo servo;

 public:
  CanServoBus(ikarashiCAN_mk2 &can, size_t n) : can(can), servo(&can, n) {}

  int Send() { return servo.send(); }

  CanServo *NewNode(size_t n);
};

class CanServo : public robotics::Node<float> {
 public:
  CanServo(can_servo &servo, size_t n) {
    SetChangeCallback([&servo, n](float value) { servo.set(n, value); });
  }
};

CanServo *CanServoBus::NewNode(size_t n) { return new CanServo(servo, n); }
}  // namespace nhk2024b::common