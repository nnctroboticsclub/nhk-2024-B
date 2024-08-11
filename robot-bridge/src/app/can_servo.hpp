#pragma once

#include <ikarashiCAN_mk2.h>
#include <can_servo.h>

namespace nhk2024b::common {
class CanServo;

class CanServoBus {
  ikarashiCAN_mk2 &can;
  can_servo servo;

 public:
  CanServoBus(ikarashiCAN_mk2 &can, int id) : can(can), servo(&can, id) {}

  int Send() { return servo.send(); }

  CanServo *NewNode(int i);
};

class CanServo : public robotics::Node<float> {
 public:
  CanServo(can_servo &servo, size_t i) {
    SetChangeCallback([&servo, i](float value) { servo.set(i, value); });
  }
};

CanServo *CanServoBus::NewNode(int i) { return new CanServo(servo, i); }
}  // namespace nhk2024b::common