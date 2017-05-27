# Instructions

This document describes the instruction encoding and all semantics.

# Environments

An environment is fixed-size buffer being able to hold a fixed amount of values.

# Instruction list

| Name                 | Arguments       | Description                                                                     |
|----------------------|-----------------|---------------------------------------------------------------------------------|
| create_environment   | size            | Create new environment for `size` values and push the identifier onto the stack |
| create_value         | type, immediate | Create a new value of `type` and push it onto the stack                         |
| write_to_environment | level, index    | Pop a value off the stack and write it to an environment                        |
