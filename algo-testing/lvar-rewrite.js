// How to implement this in C++
//
// AST::Block:
// - uint32_t blockid; // Global unique identifier for a block
//
// AST::Identifier
// - int32_t depth; // How many environment frames to climb
// - int32_t index; // At which offset the variable is stored

const util = require('util')

/*

function(export) {
  let foo;
  let bar;

  myglobal1;
  myglobal2;

  {
    let foo;
    foo;
  }

  function(a, b) {
    let foo;
    arguments;
    a;
    b;
    foo;
    bar;
  }

  {
    let foo;
    foo;
  }

  foo;
}

When run through the rewriter, it would be annoted like this

function(export) {
  let foo;
  let bar;

  export;       // { 0, 1 }
  myglobal1;    // { 1, 0 }
  myglobal2;    // { 1, 1 }

  {
    let foo;
    foo;        // { 0, 4 }
  }

  function(a, b) {
    const foo;
    arguments;  // { 0, 0 }
    a;          // { 0, 1 }
    b;          // { 0, 2 }
    foo;        // { 0, 3 }
    foo = bar;  // { 0, 3 } = { 1, 3 }
    bar;        // { 1, 3 }
  }

  {
    let foo;
    foo;        // { 0, 5 }
  }

  foo;          // { 0, 2 }
}

*/
const parse_tree = {
  type: "function",
  parameters: ["export"],
  block: {
    type: "block",
    blockid: 0,
    children: [
      {
        type: "block",
        blockid: 1,
        children: [
          {
            type: "lvar_decl",
            name: "foo"
          }
        ]
      },
      {
        type: "block",
        blockid: 2,
        children: [
          {
            type: "block",
            blockid: 3,
            children: [
              {
                type: "identifier",
                value: "foo"
              }
            ]
          }
        ]
      }
    ]
  }
}

class LVarScope {
  constructor(parent) {
    this.parent = parent;
    this.table = {}
    this.next_frame_index = 0;
  }
};

class LVarRewritter {
  constructor() {
    this.depth = 0;
    this.blockid = 0;
    this.lvar_scope = new LVarScope(undefined, 0);
    this.errors = [];
  }

  visit(node) {
    if (!node) return;

    switch (node.type) {
      case "function": {
        this.depth += 1;
        this.lvar_scope = new LVarScope(this.lvar_scope);

        let blockid_backup = this.blockid;
        this.blockid = node.blockid;

        // Declare all arguments
        this.declare("arguments");
        node.parameters.map((param) => {
          this.declare(param);
        });

        node.block.children.map((statement) => {
          this.visit(statement);
        });
        this.blockid = blockid_backup;

        node.compiler_info = {
          lvar_count: Object.keys(this.lvar_scope.table).length
        };

        this.lvar_scope = this.lvar_scope.parent;
        this.depth -= 1;

        break;
      }

      case "block": {
        let blockid_backup = this.blockid;
        this.blockid = node.blockid;
        node.children.map((statement) => {
          this.visit(statement);
        });
        this.blockid = blockid_backup;

        break;
      }

      case "lvar_decl": {

        // Check if this is a duplicate declaration
        const entry = this.resolve_symbol(node.name, true);
        if (entry) {
          this.push_error([node, "Duplicate declaration: " + node.name]);
          return;
        }

        node.compiler_info = this.declare(node.name, node.is_constant);

        break;
      }

      case "identifier": {
        const entry = this.resolve_symbol(node.value);
        if (!entry) {
          this.push_error([node, "Undefined symbol: " + node.value]);
          return;
        }

        node.compiler_info = entry;

        break;
      }

      case "assignment": {
        const target = this.resolve_symbol(node.target);
        if (!target) {
          this.push_error([node, "Undefined symbol: " + node.target]);
          return;
        }

        if (target.is_constant) {
          this.push_error([node, "Variable is constant: " + node.target])
          return;
        }

        node.compiler_info = target;

        this.visit(node.expression);
        break;
      }
    }
  }

  declare(symbol, is_constant = false) {
    const compiler_info = {
      depth: this.depth,
      blockid: this.blockid,
      frame_index: this.lvar_scope.next_frame_index++,
      is_constant: is_constant
    };
    if (!this.lvar_scope.table[symbol]) this.lvar_scope.table[symbol] = [];
    this.lvar_scope.table[symbol].push(compiler_info);
    return compiler_info;
  }

  resolve_symbol(symbol, noparentblocks = false) {

    // Search parameters
    let search_table = this.lvar_scope;
    let search_depth = this.depth;
    let search_blockid = this.blockid;

    // Iterate over all search tables
    while (search_table) {

      // Check if this table contains this variable
      if (search_table.table[symbol]) {

        // Get the records of all variables with this name under this scope
        const records = Array.from(search_table.table[symbol]);
        let matching_record = undefined;
        records.reverse().map((record, index) => {
          if (matching_record) return;

          // Direct hit inside this block
          if (record.depth == search_depth && record.blockid == search_blockid) {
            matching_record = record;
          }

          // Check if this record is reachable
          if (record.depth <= search_depth && !noparentblocks) {
            matching_record = record;
          }
        });

        // Return the matching record if one was found
        if (matching_record) return {
          level_distance: this.depth - matching_record.depth,
          index: matching_record.frame_index,
          is_constant: matching_record.is_constant
        }
      }

      if (noparentblocks) break;

      // Move up to the next table
      search_table = search_table.parent;
    }

    return undefined;
  }

  push_error(error) {
    if (this.errors.length >= 5) {
      throw this;
    }

    this.errors.push(error);
  }

  encode_symbol(symbol, depth = this.depth, blockid = this.current_blockid) {
    return "%" + depth + ":" + blockid + ":" + symbol;
  }
}

const rewriter = new LVarRewritter();

rewriter.declare("myglobal1");
rewriter.declare("myglobal2");

rewriter.visit(parse_tree);

console.log("");
console.log("Errors");
rewriter.errors.map((error) => {
  console.log(error);
});

console.log("");
console.log("Parse Tree:");
console.log(util.inspect(parse_tree, false, null))
