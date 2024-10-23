// Robobus API

class Serializable {
  void SerializeTo(Stream<uint8_t> st);
};

class DebugTree: public Serializable {
  DebugTree(uint32_t id, String name);
  DebugNode Node(String name);
  DebugTree Tree(String name);

  void SendTo(Stream<uint8_t> st) override;
};

class DebugNode: public Serializable {
  DebugNode(uint32_t id, String name, String initial_text);
  void Set(String text);
  uint32_t Id();

  void SendTo(Stream<uint8_t> st) override;
};

class Module {
  Module();

  Node<T> UseProperty<T>();
  DebugTree DebugTree();
  Module Child(String name);


};

