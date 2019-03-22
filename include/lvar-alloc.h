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
    AST::Function* function_node = nullptr;
    FunctionScope* parent_scope = nullptr;

    FunctionScope(AST::Function* fn, FunctionScope* ps) : function_node(fn), parent_scope(ps) {
    }

    ~FunctionScope() {
      if (this->function_node) {
        this->function_node->lvarcount = this->active_slots.size();
      }
    }

    uint32_t alloc_slot(bool constant);
    void mark_as_free(uint32_t index);
    inline void mark_as_leaked(uint32_t index);
  };

  struct LocalOffsetInfo {
    ValueLocation location = { .type = Location::Frame, .as_frame = { 0xFFFFFFFF, 0xFFFFFFFF } };
    bool valid = true;
    bool constant = false;
  };

  class LocalScope {
  private:
    FunctionScope* parent_function;
    LocalScope* parent_scope;
    std::map<size_t, LocalOffsetInfo> locals;

    LocalScope(FunctionScope* cf, LocalScope* ps) : contained_function(cf), parent_scope(ps) {
    }

    ~LocalScope() {
      for (auto& offset_info : this->local_indices) {
        this->contained_function->mark_as_free(offset_info.second.offset);
      }
    }

    LocalOffsetInfo alloc_slot(size_t symbol, bool constant = false);
    LocalOffsetInfo declare_slot(size_t symbol, bool constant = false);
    bool scope_contains_symbol(size_t symbol);
    LocalOffsetInfo register_symbol(size_t symbol, LocalOffsetInfo info);
    LocalOffsetInfo resolve_symbol(size_t symbol, bool ignore_parents = false);
  };

  // Allocates variables and temporary values
  class LVarAllocator {
  private:
  };

}
