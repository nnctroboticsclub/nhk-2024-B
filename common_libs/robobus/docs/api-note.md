# Robobus API

```cpp
class Serializable {
  void SerializeTo(Stream<uint8_t> st);
};

class DebugTree: public Serializable {
  DebugTree(uint32_t id, String name);
  DebugNode Node(String name, String initial_text);
  DebugTree Tree(String name);

  // Some Serializable members
};

class DebugNode: public Serializable {
  DebugNode(uint32_t id, String name, String initial_text);
  void Update(String text);
  uint32_t Id();

  // Some Serializable members
};

class Context {
  T &UseProperty<T>(uint8_t id); // --> NVN [ModuleID: 8bit][instance_id: 16bit][id: 8bit]

  DebugTree RootDebugTree();
  DebugTree DebugTree();
  Module Child(String name);

  std::shared_ptr<Controller> AcquireDevice<Controller>(uint32_t id);
};


// ↓ モジュール例

Module test_mod(Context ctx, struct {
  Joystick move;
} input) {
  auto motor_1 = co_await ctx.AcquireDevice<VelocityController>(0x41)
  auto motor_2 = co_await ctx.AcquireDevice<VelocityController>(0x42)

  input.move.Watch([](Vector2D const& stick){
    DebugTree().Node("Move", "-----").Set(stick.ToString());

    motor_1 = (stick[0] - stick[1]) / 1.41;
    motor_2 = (stick[0] + stick[1]) / 1.41;
  });
}