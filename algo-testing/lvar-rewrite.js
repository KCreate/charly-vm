const parse_tree = {
  type: "function",
  body: [
    {
      type: "lvar-decl",
      name: "foo",
      expression: {
        type: "int",
        value: 0
      }
    },
    {
      type: "lvar-decl",
      name: "baz",
      expression: {
        type: "int",
        value: 10
      }
    },
    {
      type: "scope",
      body: [
        {
          type: "lvar-decl",
          name: "foo",
          expression: {
            type: "int",
            value: 300
          }
        },
        {
          type: "assignment",
          name: "foo",
          expression: {
            type: "int",
            value: 3000
          }
        },
        {
          type: "assignment",
          name: "baz",
          expression: {
            type: "int",
            value: 100
          }
        }
      ]
    },
    {
      type: "assignment",
      name: "baz",
      expression: {
        type: "int",
        value: 400
      }
    }
  ]
}

function raise_lvars(parse_tree) {
  const lvar_table = {};
  let current_depth = 0;

  visit_body(parse_tree.body)

  Object.keys(lvar_table).reverse().map((key) => {
    parse_tree.body.unshift({
      type: "lvar-decl",
      name: key,
      expression: undefined
    });
  })

  function visit_body(body) {
    current_depth++;

    body.map((statement) => {
      if (statement.type == "lvar-decl") {

        // Check if this variable was already declared in this scope
        entry = lvar_table["%" + current_depth + ":" + statement.name]

        if (!entry) {
          lvar_table["%" + current_depth + ":" + statement.name] = true;
          statement.type = "assignment";
          statement.name = "%" + current_depth + ":" + statement.name;
        } else {
          statement.error = "Redeclaration of: " + statement.name;
        }

        return
      }

      if (statement.type == "assignment") {

        // Check if this variable was already declared somewhere higher up
        let entry = undefined
        let i = 0;
        for (i = current_depth; i >= 0; i--) {
          entry = lvar_table["%" + i + ":" + statement.name]
          if (entry) break;
        }

        if (!entry) {
          statement.error = "Undefined variable: " + statement.name;
        } else {
          statement.name = "%" + i + ":" + statement.name;
        }

        return
      }

      if (statement.type == "scope") {
        visit_body(statement.body)
        return
      }
    })

    current_depth--;
  }
}

raise_lvars(parse_tree)

const util = require('util')

console.log(util.inspect(parse_tree, false, null))
































