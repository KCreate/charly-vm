#define AST_TYPESWITCH_CASE_NODE(NodeType, ...)         \
  case Node::Type::NodeType: {                          \
    ref<NodeType> node = cast<NodeType>(original_node); \
    __VA_ARGS__                                         \
    break;                                              \
  }

#define AST_TYPESWITCH(OriginalNode, ...)                     \
  {                                                           \
    const ref<Node>& original_node = OriginalNode;            \
    switch (original_node->type()) {                          \
      AST_TYPESWITCH_CASE_NODE(Program, __VA_ARGS__)          \
      AST_TYPESWITCH_CASE_NODE(Block, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Return, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(Break, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Continue, __VA_ARGS__)         \
      AST_TYPESWITCH_CASE_NODE(Defer, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Throw, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Export, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(Import, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(ImportExpression, __VA_ARGS__) \
      AST_TYPESWITCH_CASE_NODE(Yield, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Spawn, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Await, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Typeof, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(As, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Id, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(Int, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(Float, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Bool, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(Char, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(String, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(FormatString, __VA_ARGS__)     \
      AST_TYPESWITCH_CASE_NODE(Null, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(Self, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(Super, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Tuple, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(List, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(DictEntry, __VA_ARGS__)        \
      AST_TYPESWITCH_CASE_NODE(Dict, __VA_ARGS__)             \
      AST_TYPESWITCH_CASE_NODE(Function, __VA_ARGS__)         \
      AST_TYPESWITCH_CASE_NODE(Class, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(ClassProperty, __VA_ARGS__)    \
      AST_TYPESWITCH_CASE_NODE(Assignment, __VA_ARGS__)       \
      AST_TYPESWITCH_CASE_NODE(Ternary, __VA_ARGS__)          \
      AST_TYPESWITCH_CASE_NODE(BinaryOp, __VA_ARGS__)         \
      AST_TYPESWITCH_CASE_NODE(UnaryOp, __VA_ARGS__)          \
      AST_TYPESWITCH_CASE_NODE(Spread, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(CallOp, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(MemberOp, __VA_ARGS__)         \
      AST_TYPESWITCH_CASE_NODE(IndexOp, __VA_ARGS__)          \
      AST_TYPESWITCH_CASE_NODE(Declaration, __VA_ARGS__)      \
      AST_TYPESWITCH_CASE_NODE(If, __VA_ARGS__)               \
      AST_TYPESWITCH_CASE_NODE(While, __VA_ARGS__)            \
      AST_TYPESWITCH_CASE_NODE(Try, __VA_ARGS__)              \
      AST_TYPESWITCH_CASE_NODE(Switch, __VA_ARGS__)           \
      AST_TYPESWITCH_CASE_NODE(SwitchCase, __VA_ARGS__)       \
      AST_TYPESWITCH_CASE_NODE(For, __VA_ARGS__)              \
      case Node::Type::Unknown:                               \
      default: {                                              \
        assert(false && "unexpected node type");              \
        break;                                                \
      }                                                       \
    }                                                         \
  }

#define HANDLE_NODE(ReplacementType, NodeType)                        \
public:                                                               \
  virtual ref<ReplacementType> apply(const ref<NodeType>& node) {     \
    if (node.get() == nullptr)                                        \
      return nullptr;                                                 \
    enter(node);                                                      \
    bool visit_children = inspect_enter(node);                        \
    m_depth++;                                                        \
    if (visit_children)                                               \
      children(node);                                                 \
    m_depth--;                                                        \
    ref<ReplacementType> replaced_node = transform(node);             \
    if (replaced_node) {                                              \
      AST_TYPESWITCH(replaced_node, { inspect_leave(node); });        \
      leave(replaced_node);                                           \
    }                                                                 \
    return replaced_node;                                             \
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
  void children([[maybe_unused]] const ref<NodeType>& node)

#define VISIT_NODE(N)                                                \
  if (node->N) {                                                     \
    node->N = cast<decltype(node->N)::element_type>(apply(node->N)); \
  }

#define VISIT_NODE_VECTOR(N)                                      \
  {                                                               \
    using NodeType = decltype(node->N)::value_type::element_type; \
    for (ref<NodeType> & child_node : node->N) {                  \
      child_node = cast<NodeType>(this->apply(child_node));       \
    }                                                             \
    auto begin = node->N.begin();                                 \
    auto end = node->N.end();                                     \
    node->N.erase(std::remove(begin, end, nullptr), end);         \
  }
