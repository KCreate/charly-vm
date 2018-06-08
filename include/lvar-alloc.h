namespace Charly {

  // Different types of locations
  enum LocationType : uint8_t {
    Frame,
    Stack,
    Arguments
  };

  // Stores the location of a value
  struct ValueLocation {
    LocationType type;

    union {
      struct {
        uint32_t level;
        uint32_t index;
      } as_frame;

      struct {
        uint32_t offset;
      } as_arguments;
    };
  };

  // Holds information about a given slot in a function's lvar table
  struct SlotInfo {
    bool active = true;
    bool leaked = false;
    bool constant = false;
  };

  class FunctionScope {
  private:
    std::vector<SlotInfo> slots;
    std::map<size_t, size_t> symbols;
    AST::Function* function_node = nullptr;
  };

  struct LocalOffsetInfo {
    uint32_t level = 0xFFFFFFFF;
    uint32_t offset = 0xFFFFFFFF;
    bool valid = true;
    bool constant = false;
  };

  class LocalScope {
  private:
    FunctionScope* parent_function;
    LocalScope* parent_scope;
    std::map<size_t, LocalOffsetInfo> locals;
  };

  // Allocates variables and temporary values
  class LVarAllocator {
  private:
  };

}
