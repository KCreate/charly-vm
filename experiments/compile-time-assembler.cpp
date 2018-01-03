#include <iostream>
#include <iomanip>
#include <utility>
#include <cstdint>
#include <array>
#include <unordered_map>
#include <type_traits>

struct __attribute__((packed)) readlocal {
    uint8_t opcode = 0x00;
    uint32_t level;
    uint32_t index;
};

struct __attribute__((packed)) setlocal {
    uint8_t opcode = 0x01;
    uint32_t level;
    uint32_t index;
};

struct __attribute__((packed)) add {
    uint8_t opcode = 0x02;
};

struct __attribute__((packed)) branch {
    uint8_t opcode = 0x03;
    uint32_t label;
};

struct label {
    uint32_t id;
};

template <typename... Args>
static auto assemble(Args&&... params) {

    // Calculate the size of the buffer
    const size_t size = (0 + ... + (std::is_same<Args, label>::value ? 0 : sizeof(params)));

    // Table for unresolved offsets
    uint32_t encountered_labels[sizeof...(params)] = { 0xffffffff };
    uint32_t unresolved_offsets[sizeof...(params)] = { 0xffffffff };

    // The buffer which holds the instructions
    static std::array<char, size> buf = { 0 };

    // The write offset into the buffer
    uint32_t woff = 0;

    // Loop over each instruction to assemble
    (void([&]() {

        // If this is a label, we register it in the encountered_labels table
        // and jump to the next instruction
        if constexpr(std::is_same<Args, label>::value) {
            encountered_labels[params.id] = woff;
            return;
        }

        // If this is a branch instruction, we check if we already have an offset
        // or if we need to save it in the unresolved offsets table
        if constexpr(std::is_same<Args, branch>::value) {
            if (encountered_labels[params.label] != 0xffffff) {
                *reinterpret_cast<Args*>(buf.data() + (woff += sizeof(Args)) - sizeof(Args)) = params;
                *reinterpret_cast<uint32_t*>(buf.data() + woff - 4) = (woff - 5) - encountered_labels[params.label];
            } else {
                unresolved_offsets[woff] = params.label;
                *reinterpret_cast<Args*>(buf.data() + (woff += sizeof(Args)) - sizeof(Args)) = params;
                *reinterpret_cast<uint32_t*>(buf.data() + woff - 4) = 0xffffff;
            }
        } else {
            *reinterpret_cast<Args*>(buf.data() + (woff += sizeof(Args)) - sizeof(Args)) = params;
        }
    }()), ...);

    // Resolve all unresolved labels
    for (int i = 0; i < size; i++) {

      // Check if this address has a label it needs
      if (unresolved_offsets[i] != 0xffffffff) {
        uint32_t lbl = unresolved_offsets[i];

        // Check if we known such a label
        if (uint32_t address = encountered_labels[lbl]; address != 0xffffffff) {
          *reinterpret_cast<uint32_t*>(buf.data() + i + 1) = address - i;
        }
      }
    }

    return buf;
}

int main() {
    auto blk = assemble(
        label { 0x1 },
        setlocal { .level = 3, .index = 2},
        branch { .label = 0x1 },
        branch { .label = 0x1 }
    );

    for (size_t i = 0; i < blk.size(); i++) {
        std::cout << std::hex << std::setw(2);
        std::cout << "0x" << static_cast<unsigned int>(blk[i]) << ' ';
    }

    return blk.size();
}
