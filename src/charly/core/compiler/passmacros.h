#include <functional>

#define AST_TYPESWITCH_CASE_NODE(NodeType, ...)         \
  case Node::Type::NodeType: {                          \
    ref<NodeType> node = cast<NodeType>(original_node); \
    __VA_ARGS__                                         \
    break;                                              \
  }

#define AST_TYPESWITCH(OriginalNode, ...)                        \
  {                                                              \
    const ref<Node>& original_node = OriginalNode;               \
    switch (original_node->type()) {                             \
      AST_TYPESWITCH_CASE_NODE(Block, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Return, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(Break, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Continue, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Throw, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Export, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(Import, __VA_ARGS__)              \
                                                                 \
      AST_TYPESWITCH_CASE_NODE(Yield, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Spawn, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Await, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Typeof, __VA_ARGS__)              \
                                                                 \
      AST_TYPESWITCH_CASE_NODE(Id, __VA_ARGS__)                  \
      AST_TYPESWITCH_CASE_NODE(Name, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(Int, __VA_ARGS__)                 \
      AST_TYPESWITCH_CASE_NODE(Float, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Bool, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(Char, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(String, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(FormatString, __VA_ARGS__)        \
      AST_TYPESWITCH_CASE_NODE(Symbol, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(Null, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(Self, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(FarSelf, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(Super, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Tuple, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(List, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(DictEntry, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(Dict, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(FunctionArgument, __VA_ARGS__)    \
      AST_TYPESWITCH_CASE_NODE(Function, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(ClassProperty, __VA_ARGS__)       \
      AST_TYPESWITCH_CASE_NODE(Class, __VA_ARGS__)               \
                                                                 \
      AST_TYPESWITCH_CASE_NODE(MemberOp, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(IndexOp, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(UnpackTargetElement, __VA_ARGS__) \
      AST_TYPESWITCH_CASE_NODE(UnpackTarget, __VA_ARGS__)        \
      AST_TYPESWITCH_CASE_NODE(Assignment, __VA_ARGS__)          \
      AST_TYPESWITCH_CASE_NODE(Ternary, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(BinaryOp, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(UnaryOp, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(Spread, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(CallOp, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(Declaration, __VA_ARGS__)         \
      AST_TYPESWITCH_CASE_NODE(UnpackDeclaration, __VA_ARGS__)   \
      AST_TYPESWITCH_CASE_NODE(If, __VA_ARGS__)                  \
      AST_TYPESWITCH_CASE_NODE(While, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Loop, __VA_ARGS__)                \
      AST_TYPESWITCH_CASE_NODE(Try, __VA_ARGS__)                 \
      AST_TYPESWITCH_CASE_NODE(TryFinally, __VA_ARGS__)          \
      AST_TYPESWITCH_CASE_NODE(SwitchCase, __VA_ARGS__)          \
      AST_TYPESWITCH_CASE_NODE(Switch, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(For, __VA_ARGS__)                 \
      AST_TYPESWITCH_CASE_NODE(BuiltinOperation, __VA_ARGS__)    \
      case Node::Type::Unknown:                                  \
      default: {                                                 \
        FAIL("unexpected node type in AST_TYPESWITCH");          \
      }                                                          \
    }                                                            \
  }

#define APPLY_NODE(N)                                             \
  {                                                               \
    if (node->N) {                                                \
      using NodeType = decltype(node->N)::element_type;           \
      ref<NodeType> replacement = cast<NodeType>(apply(node->N)); \
      if (replacement.get() != node->N.get()) {                   \
        node->N = replacement;                                    \
      }                                                           \
    }                                                             \
  }

#define APPLY_VECTOR(N)                                              \
  {                                                                  \
    using NodeType = decltype(node->N)::value_type::element_type;    \
    for (ref<NodeType> & child_node : node->N) {                     \
      ref<NodeType> replacement = cast<NodeType>(apply(child_node)); \
      if (replacement.get() != child_node.get()) {                   \
        child_node = replacement;                                    \
      }                                                              \
    }                                                                \
    auto begin = node->N.begin();                                    \
    auto end = node->N.end();                                        \
    node->N.erase(std::remove(begin, end, nullptr), end);            \
  }

#define HANDLE_NODE(ReplacementType, NodeType, Children)              \
public:                                                               \
  virtual ref<ReplacementType> apply(const ref<NodeType>& node) {     \
    if (node.get() == nullptr)                                        \
      return nullptr;                                                 \
    enter(node);                                                      \
    bool visit_children = inspect_enter(node);                        \
    m_depth++;                                                        \
    if (visit_children)                                               \
      apply_children(node);                                           \
    m_depth--;                                                        \
    ref<ReplacementType> replaced_node = transform(node);             \
    if (replaced_node.get() == nullptr)                               \
      return nullptr;                                                 \
    if (replaced_node.get() != node.get()) {                          \
      return apply(replaced_node);                                    \
    } else {                                                          \
      inspect_leave(node);                                            \
      leave(node);                                                    \
    }                                                                 \
    return node;                                                      \
  }                                                                   \
                                                                      \
private:                                                              \
  virtual bool inspect_enter(const ref<NodeType>&) {                  \
    return true;                                                      \
  }                                                                   \
  virtual void inspect_leave(const ref<NodeType>&) {}                 \
  virtual ref<ReplacementType> transform(const ref<NodeType>& node) { \
    return node;                                                      \
  }                                                                   \
  void apply_children([[maybe_unused]] const ref<NodeType>& node) {   \
    Children;                                                         \
  }
