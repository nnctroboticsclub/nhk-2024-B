#pragma once

#include <robotics/logger/logger.hpp>
#include <robotics/node/node.hpp>
#include "ps4_con.hpp"
#include "types.hpp"

namespace vs_ps4 {
robotics::logger::Logger logger{"vs_ps4", "   PS4   "};

robotics::Node<robotics::types::JoyStick2D> stick_left;
robotics::Node<robotics::types::JoyStick2D> stick_right;
robotics::Node<nhk2024b::ps4_con::DPad> dpad;
robotics::Node<bool> button_square;
robotics::Node<bool> button_cross;
robotics::Node<bool> button_circle;
robotics::Node<bool> button_triangle;
robotics::Node<bool> button_share;
robotics::Node<bool> button_options;
robotics::Node<bool> button_ps;
robotics::Node<bool> button_touchPad;
robotics::Node<bool> button_l1;
robotics::Node<bool> button_r1;
robotics::Node<bool> button_l3;
robotics::Node<bool> button_r3;
robotics::Node<float> trigger_l;
robotics::Node<float> trigger_r;
robotics::Node<float> battery_level;

namespace state {
robotics::types::JoyStick2D stick_left_value;
robotics::types::JoyStick2D stick_right_value;
nhk2024b::ps4_con::DPad dpad_value = nhk2024b::ps4_con::DPad::kNone;
bool button_square_value = false;
bool button_cross_value = false;
bool button_circle_value = false;
bool button_triangle_value = false;
bool button_share_value = false;
bool button_options_value = false;
bool button_ps_value = false;
bool button_touchPad_value = false;
bool button_l1_value = false;
bool button_r1_value = false;
bool button_l3_value = false;
bool button_r3_value = false;
float trigger_l_value = 0;
float trigger_r_value = 0;
float battery_level_value = 0;

class GenericEntry {
 public:
  virtual int Dirtyness() = 0;
  virtual void Update() = 0;
  virtual void Invalidate() = 0;
};

template <typename T>
class Entry : public GenericEntry {
  T &raw_value;
  robotics::Node<T> &node;
  int dirtyness = 0;

 public:
  Entry(T &value, robotics::Node<T> &node) : raw_value(value), node(node) {}

  int Dirtyness() override { return dirtyness; }

  void Update() override {
    if (raw_value != node.GetValue()) {
      dirtyness++;
    } else {
      dirtyness = 0;
    }
  }

  void Invalidate() override {
    node.SetValue(raw_value);
    dirtyness = 0;
  }
};

struct Entries {
  std::vector<GenericEntry *> entries;

  GenericEntry *FindMostDirtyEntry() {
    int dirtiest = 0;
    GenericEntry *dirtiest_entry = nullptr;

    for (auto entry : entries) {
      auto dirtyness = entry->Dirtyness();
      if (dirtyness > dirtiest) {
        dirtiest = dirtyness;
        dirtiest_entry = entry;
      }
    }

    return dirtiest_entry;
  }

  int DirtyEntries() {
    int result = 0;

    for (auto entry : entries) {
      if (entry->Dirtyness() > 0) {
        result += 1;
      }
    }

    return result;
  }

  void Update() {
    for (auto entry : entries) {
      entry->Update();
    }
  }

  void Send() {
    auto entry = FindMostDirtyEntry();
    if (!entry) {
      return;
    }
    entry->Invalidate();
  }

  void Add(GenericEntry *entry) { entries.emplace_back(entry); }
};

Entries *entries_1 = nullptr;
Entries *entries_2 = nullptr;
Entries *entries_other = nullptr;

void Init() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  entries_1 = new Entries();
  entries_2 = new Entries();
  entries_other= new Entries();

  entries_1->Add(
      new Entry<robotics::types::JoyStick2D>(stick_left_value, stick_left));
  entries_2->Add(
      new Entry<robotics::types::JoyStick2D>(stick_right_value, stick_right));
  entries_1->Add(new Entry<nhk2024b::ps4_con::DPad>(dpad_value, dpad));
  entries_2->Add(new Entry<bool>(button_square_value, button_square));
  entries_2->Add(new Entry<bool>(button_cross_value, button_cross));
  entries_2->Add(new Entry<bool>(button_circle_value, button_circle));
  entries_2->Add(new Entry<bool>(button_triangle_value, button_triangle));
  entries_1->Add(new Entry<bool>(button_share_value, button_share));
  entries_2->Add(new Entry<bool>(button_options_value, button_options));
  entries_other->Add(new Entry<bool>(button_ps_value, button_ps));
  // entries->Add(
  //     new Entry<bool>(button_touchPad_value, button_touchPad));
  entries_2->Add(new Entry<bool>(button_l1_value, button_l1));
  entries_2->Add(new Entry<bool>(button_r1_value, button_r1));
  // entries->Add(new Entry<bool>(button_l3_value, button_l3));
  // entries->Add(new Entry<bool>(button_r3_value, button_r3));
  entries_1->Add(new Entry<float>(trigger_l_value, trigger_l));
  entries_1->Add(new Entry<float>(trigger_r_value, trigger_r));
  // entries->Add(new Entry<float>(battery_level_value, battery_level));
}

}  // namespace state

};  // namespace vs_ps4