import sys
import time

# instruction set
OP_LOAD = 1             # push immediate number
OP_ADD = 2              # add top two values of stack
OP_PRINT = 3            # pop value from stack and print
OP_JMP = 4              # jump to offset
OP_JMPIF = 5            # jump to offset if popped value is not zero
OP_CMP = 6              # check if two values are equal, pushes 1 or 0
OP_SETUPCATCH = 7       # setup a catch handler at some offset
OP_SETUPDEFER = 8       # setup a defer handler at some offset
OP_UNWIND = 9           # unwind a block
OP_DEFERCONTINUE = 10   # exit a defer block
OP_THROW = 11           # throw a value
OP_CALL = 12            # call a function at a given offset
OP_RET = 13             # return from a function
OP_WRITELOCAL = 14      # write to a local variable
OP_LOADLOCAL = 15       # read from a local variable
OP_GETNUMBER = 16       # read a number from the user
OP_EXIT = 17            # exit with top value as exit code
OP_ABORT = 18           # abort the vm
OP_DUP = 19             # duplicate top value of the stack

opcode_names = [
    "-",
    "load",
    "add",
    "print",
    "jmp",
    "jmpif",
    "cmp",
    "setupcatch",
    "setupdefer",
    "unwind",
    "defercontinue",
    "throw",
    "call",
    "ret",
    "writelocal",
    "loadlocal",
    "getnumber",
    "exit",
    "abort",
    "dup"
]

code_memory = [
    ".main",
        OP_GETNUMBER,
        OP_GETNUMBER,
        OP_WRITELOCAL, 1,
        OP_WRITELOCAL, 0,

        OP_LOAD, 42,
        OP_WRITELOCAL, 2,

        OP_SETUPCATCH, "main#L3", "main#L4",
        OP_SETUPDEFER, "main#L1", "main#L2",
        OP_LOADLOCAL, 0,
        OP_LOADLOCAL, 1,
        OP_CALL, "prevent_14",
        OP_WRITELOCAL, 2,
        OP_UNWIND,

    ".main#L1",
        OP_LOADLOCAL, 2,
        OP_PRINT,
        OP_DEFERCONTINUE,

    ".main#L2",
        OP_UNWIND,

    ".main#L3",
        OP_LOAD, 3333,
        OP_WRITELOCAL, 2,

    ".main#L4",
        OP_LOADLOCAL, 2,
        OP_EXIT,

    ".add",
        OP_SETUPDEFER, "add#L3", "add#L4",
        OP_SETUPCATCH, "add#L1", "add#L2",
        OP_ADD,
        OP_THROW,

    ".add#L1",
        OP_LOAD, "inside add catch handler",
        OP_PRINT,
        OP_DUP,
        OP_PRINT,
        OP_RET,

    ".add#L2", # unreachable
        OP_ABORT,

    ".add#L3",
        OP_LOAD, "inside add defer block",
        OP_PRINT,
        OP_DEFERCONTINUE,

    ".add#L4", # unreachable
        OP_ABORT,

    ".prevent_14",
        OP_SETUPDEFER, "prevent_14#L1", "prevent_14#L2",
        OP_CALL, "add",
        OP_WRITELOCAL, 0,
        OP_LOADLOCAL, 0,
        OP_LOAD, 14,
        OP_CMP,
        OP_JMPIF, "prevent_14#L3",
        OP_LOADLOCAL, 0,
        OP_RET,

    ".prevent_14#L1",
        OP_LOAD, "inside prevent 14 defer block",
        OP_PRINT,
        OP_DEFERCONTINUE,

    ".prevent_14#L2", #unreachable
        OP_ABORT,

    ".prevent_14#L3",
        OP_THROW
];

UNWIND_EXCEPTION = 0    # unwinding to the last catch handler
UNWIND_RETURN = 1       # unwinding to return the function

data_stack = []

function_stack = [["err", [0, 0, 0, 0], None]]

block_stack = []

unwind_reason = None

instruction_ptr = 0

exit_code = 0

def incip(n):
    global instruction_ptr
    instruction_ptr += n

def resolve_label(label):
    global code_memory
    offset = code_memory.index(f".{label}")
    return offset

def last_block(block_type):
    global function_stack
    for block in reversed(block_stack):
        if block[0] == block_type:
            return block
    return None

def push_block(block_type, handler_label, continue_label):
    parent_block = last_block(block_type)
    active_frame = function_stack[-1]

    handler_offset = resolve_label(handler_label)
    if continue_label != None:
        continue_offset = resolve_label(continue_label)
        record = (block_type, handler_offset, continue_offset, parent_block, active_frame)
    else:
        record = (block_type, handler_offset, None, parent_block, active_frame)

    global block_stack
    block_stack.append(record)

def unwind_block():
    global block_stack
    global function_stack
    global instruction_ptr
    global unwind_reason

    if len(block_stack) == 0:
        raise Exception("block stack empty")

    if len(function_stack) == 0:
        raise Exception("function stack empty")

    top_frame = function_stack[-1]
    top_block = block_stack[-1]

    if unwind_reason == UNWIND_EXCEPTION:

        # unwind function frames
        while top_block[4] != top_frame:
            function_stack.pop()
            top_frame = function_stack[-1]

        if top_block[0] == "catch":
            block_stack.pop()
            unwind_reason = None
            instruction_ptr = top_block[1]
        elif top_block[0] == "defer":
            instruction_ptr = top_block[1]
        else:
            raise Exception("unexpected block type")
    elif unwind_reason == UNWIND_RETURN:
        last_defer_block = last_block("defer")

        # if there is no defer block left at all
        # we unwind all the remaining catch blocks
        # of the current frame
        if last_defer_block == None:
            while len(block_stack) and block_stack[-1][4] == top_frame:
                block_stack.pop()
            instruction_ptr = top_frame[0]
            function_stack.pop()
            return;

        # if the defer block is inside our current frame we can simply jump to it
        if last_defer_block[4] == top_frame:

            # unwind blocks up to the last defer block
            while len(block_stack) and block_stack[-1] != last_defer_block:
                block_stack.pop()
            instruction_ptr = last_defer_block[1]
            return;

        # if the defer block is outside the current frame we can
        # pop the current frame and restore the top block
        instruction_ptr = top_frame[0]
        unwind_reason = None
        function_stack.pop()

        # unwind all the remaning blocks of the frame
        while len(block_stack) and block_stack[-1][4] == top_frame:
            block_stack.pop()

    else:
        raise Exception("expected unwind reason")

while True:
    opcode = code_memory[instruction_ptr]

    # skip labels
    if type(opcode) is str:
        incip(1)
        continue

    microtime_start = time.time() * 1000 * 1000

    if opcode == OP_LOAD:
        value = code_memory[instruction_ptr + 1]
        data_stack.append(value)
        incip(2)

    elif opcode == OP_GETNUMBER:
        print("enter number: ", end="")
        sys.stdout.flush()
        data_stack.append(int(sys.stdin.readline()))
        #  data_stack.append(7)
        incip(1)

    elif opcode == OP_WRITELOCAL:
        offset = code_memory[instruction_ptr + 1]
        function_stack[-1][1][offset] = data_stack.pop()
        incip(2)

    elif opcode == OP_SETUPCATCH:
        push_block("catch", code_memory[instruction_ptr + 1], code_memory[instruction_ptr + 2])
        incip(2)

    elif opcode == OP_SETUPDEFER:
        push_block("defer", code_memory[instruction_ptr + 1], code_memory[instruction_ptr + 2])
        incip(3)

    elif opcode == OP_JMP:
        instruction_ptr = resolve_label(code_memory[instruction_ptr + 1])

    elif opcode == OP_JMPIF:
        value = data_stack.pop()
        if value:
            instruction_ptr = resolve_label(code_memory[instruction_ptr + 1])
        else:
            incip(2)

    elif opcode == OP_LOADLOCAL:
        offset = code_memory[instruction_ptr + 1]
        value = function_stack[-1][1][offset]
        data_stack.append(value)
        incip(2)

    elif opcode == OP_PRINT:
        value = data_stack.pop()
        print(f"=> {value}")
        incip(1)

    elif opcode == OP_CALL:
        call_label = code_memory[instruction_ptr + 1]
        call_offset = resolve_label(call_label)

        top_block = block_stack[-1] if len(block_stack) else None

        function_stack.append([instruction_ptr + 2, [9999, 9999, 9999, 9999], top_block])
        instruction_ptr = call_offset

    elif opcode == OP_ADD:
        rhs = data_stack.pop()
        lhs = data_stack.pop()
        data_stack.append(lhs + rhs)
        incip(1)

    elif opcode == OP_CMP:
        rhs = data_stack.pop()
        lhs = data_stack.pop()
        data_stack.append(lhs == rhs)
        incip(1)

    elif opcode == OP_UNWIND:
        if len(block_stack) == 0:
            raise Exception("empty block stack")
        top_block = block_stack[-1]

        if top_block[0] == "catch":
            instruction_ptr = top_block[2]
            block_stack.pop()
        elif top_block[0] == "defer":
            instruction_ptr = top_block[1]
        else:
            raise Exception("unexpected block type")

    elif opcode == OP_DEFERCONTINUE:
        if len(block_stack) < 2:
            raise Exception("empty block stack")
        top_block = block_stack.pop()

        if unwind_reason != None:
            unwind_block()
        else:
            instruction_ptr = top_block[2]

    elif opcode == OP_RET:
        active_frame = function_stack[-1]
        top_block = block_stack[-1] if len(block_stack) else None

        # check if we have to unwind any blocks from this frame
        if top_block and active_frame[2] != top_block:
            unwind_reason = UNWIND_RETURN
            unwind_block()
        else:
            function_stack.pop()
            instruction_ptr = active_frame[0]

    elif opcode == OP_THROW:
        last_catch = last_block("catch")
        if last_catch == None:
            raise Exception("no exception handler found")

        unwind_reason = UNWIND_EXCEPTION
        unwind_block()

    elif opcode == OP_EXIT:
        exit_code = data_stack.pop()
        break

    elif opcode == OP_ABORT:
        raise Exception("internal error")

    elif opcode == OP_DUP:
        if len(data_stack) == 0:
            raise Exception("empty stack")

        data_stack.append(data_stack[-1])
        incip(1)

    else:
        raise Exception(f"unexpected opcode: {opcode_names[opcode]}")

    microtime_end = time.time() * 1000 * 1000
    delta = microtime_end - microtime_start

    #  print(f"| {opcode_names[opcode]} {delta}μs")

print(f"VM exited with code {exit_code}")
