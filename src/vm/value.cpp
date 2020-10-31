/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <algorithm>
#include <iomanip>

#include "value.h"
#include "symboltable.h"

namespace Charly {

void charly_debug_print(std::ostream& io, VALUE value, std::vector<VALUE>& trace) {
  bool printed_before = std::find(trace.begin(), trace.end(), value) != trace.end();

  if (printed_before) {
    io << "<";
    io << charly_get_typestring(value);
    io << " ...>";
    return;
  }

  switch (charly_get_type(value)) {
    case kTypeDead: {
      io << "<@" << charly_as_pointer(value);
      io << " : Dead>";
      break;
    }

    case kTypeNumber: {
      const double d = io.precision();
      io.precision(16);
      io << std::fixed;
      if (charly_is_int(value)) {
        io << charly_int_to_int64(value);
      } else {
        io << charly_double_to_double(value);
      }
      io << std::scientific;
      io.precision(d);

      break;
    }

    case kTypeBoolean: {
      io << (value == kTrue ? "true" : "false");
      break;
    }

    case kTypeNull: {
      io << "null";
      break;
    }

    case kTypeString: {
      io << "\"";
      io.write(charly_string_data(value), charly_string_length(value));
      io << "\"";
      break;
    }

    case kTypeObject: {
      trace.push_back(value);
      Object* object = charly_as_object(value);

      io << "<Object";
      object->access_container_shared([&](Container::ContainerType* container) {
        for (auto& entry : *container) {
          io << " ";
          std::string key = SymbolTable::decode(entry.first);
          io << key;
          io << "=";
          charly_debug_print(io, entry.second, trace);
        }
      });

      io << ">";

      trace.pop_back();
      break;
    }

    case kTypeArray: {
      trace.push_back(value);
      Array* array = charly_as_array(value);

      io << "<Array [";
      bool first = true;
      array->access_vector_shared([&](Array::VectorType* vec) {
        for (VALUE entry : *vec) {
          if (!first) {
            io << ", ";
          }

          first = false;
          charly_debug_print(io, entry, trace);
        }
      });

      io << "]>";

      trace.pop_back();
      break;
    }

    case kTypeFunction: {
      trace.push_back(value);
      Function* func = charly_as_function(value);

      io << "<Function name=";
      if (Class* host_class = func->get_host_class()) {
        charly_debug_print(io, host_class->get_name(), trace);
        io << ":";
      }
      charly_debug_print(io, func->get_name(), trace);
      io << " argc=";
      io << func->get_argc();
      io << " minimum_argc=";
      io << func->get_minimum_argc();
      io << " lvarcount=";
      io << func->get_lvarcount();
      io << " body_address=";
      io << reinterpret_cast<void*>(func->get_body_address());
      io << " ";

      VALUE bound_self;
      if (func->get_bound_self(&bound_self)) {
        io << "bound_self=";
        charly_debug_print(io, bound_self, trace);
      }

      func->access_container_shared([&](Container::ContainerType* container) {
        for (auto& entry : *container) {
          io << " ";
          std::string key = SymbolTable::decode(entry.first);
          io << key;
          io << "=";
          charly_debug_print(io, entry.second, trace);
        }
      });

      io << ">";

      trace.pop_back();
      break;
    }

    case kTypeCFunction: {
      trace.push_back(value);
      CFunction* func = charly_as_cfunction(value);

      io << "<CFunction name=";
      charly_debug_print(io, func->get_name(), trace);
      io << " argc=";
      io << func->get_argc();
      io << " pointer=";
      io << static_cast<void*>(func->get_pointer());
      io << "";

      func->access_container_shared([&](Container::ContainerType* container) {
        for (auto& entry : *container) {
          io << " ";
          std::string key = SymbolTable::decode(entry.first);
          io << key;
          io << "=";
          charly_debug_print(io, entry.second, trace);
        }
      });

      io << ">";

      trace.pop_back();
      break;
    }

    case kTypeClass: {
      trace.push_back(value);
      Class* klass = charly_as_class(value);

      io << "<Class name=";
      charly_debug_print(io, klass->get_name(), trace);
      io << " ";
      if (Function* constructor = klass->get_constructor()) {
        io << "constructor=";
        charly_debug_print(io, constructor->as_value(), trace);
        io << " ";
      }

      io << "member_properties=[";
      klass->access_member_properties([&](Class::VectorType* props) {
        for (auto entry : *props) {
          io << " ";
          io << SymbolTable::decode(entry);
        }
      });
      io << "] ";

      if (Object* prototype = klass->get_prototype()) {
        io << "member_functions=";
        charly_debug_print(io, prototype->as_value(), trace);
        io << " ";
      }

      if (Class* parent_class = klass->get_parent_class()) {
        io << "parent=";
        charly_debug_print(io, parent_class->as_value(), trace);
        io << " ";
      }

      klass->access_container_shared([&](Container::ContainerType* container) {
        for (auto entry : *container) {
          io << " ";
          io << SymbolTable::decode(entry.first);
          io << "=";
          charly_debug_print(io, entry.second, trace);
        }
      });

      io << ">";

      trace.pop_back();
      break;
    }

    case kTypeCPointer: {
      CPointer* cpointer = charly_as_cpointer(value);
      io << "<CPointer ";
      io << cpointer->get_data();
      io << ":";
      io << reinterpret_cast<void*>(cpointer->get_destructor());
      io << ">";
      break;
    }

    case kTypeSymbol: {
      io << SymbolTable::decode(value);
      break;
    }

    case kTypeFrame: {
      Frame* frame = charly_as_frame(value);
      Function* function = frame->get_function();
      VALUE name = function->get_anonymous() ? SYM("<anonymous>") : function->get_name();
      uint8_t* body_address = function->get_body_address();

      io << "(";
      io << std::setfill(' ');
      io << std::setw(14);
      io << static_cast<void*>(body_address);
      io << std::setw(1);
      io << ") ";
      io << SymbolTable::decode(name);

      break;
    }

    case kTypeCatchTable: {
      trace.push_back(value);

      CatchTable* table = charly_as_catchtable(value);
      io << "<CatchTable address=";
      io << reinterpret_cast<void*>(table->get_address());
      io << " stacksize=";
      io << table->get_stacksize();
      io << " frame=";
      io << table->get_frame();
      io << " parent=";
      io << table->get_parent();
      io << ">";

      trace.pop_back();
      break;
    }

    default: {
      io << "<unknown>";
      break;
    }
  }
}

void charly_to_string(std::ostream& io, VALUE value, std::vector<VALUE>& trace) {
  bool printed_before = std::find(trace.begin(), trace.end(), value) != trace.end();

  if (printed_before) {
    io << "<...>";
    return;
  }

  switch (charly_get_type(value)) {
    case kTypeString: {
      if (trace.size() > 0) io << "\"";
      io.write(charly_string_data(value), charly_string_length(value));
      if (trace.size() > 0) io << "\"";
      break;
    }

    case kTypeObject: {
      trace.push_back(value);

      Object* object = charly_as_object(value);
      if (Class* klass = object->get_klass()) {
        charly_to_string(io, klass->get_name());
      }

      io << "{\n";

      object->access_container_shared([&](Container::ContainerType* container) {
        for (auto& entry : *container) {
          io << std::string(((trace.size() - 1) * 2 + 2), ' ');
          std::string key = SymbolTable::decode(entry.first);
          io << key;
          io << " = ";
          charly_to_string(io, entry.second, trace);
          io << '\n';
        }
      });

      io << std::string((trace.size() - 1) * 2, ' ');
      io << "}";

      trace.pop_back();
      break;
    }

    case kTypeArray: {
      trace.push_back(value);

      Array* array = charly_as_array(value);
      io << "[";

      bool first = true;
      array->access_vector_shared([&](Array::VectorType* vec) {
        for (VALUE entry : *vec) {
          if (!first) {
            io << ", ";
          }

          first = false;
          charly_to_string(io, entry, trace);
        }
      });

      io << "]";

      trace.pop_back();
      break;
    }

    case kTypeFunction: {
      trace.push_back(value);

      Function* func = charly_as_function(value);
      io << "<Function ";

      if (Class* host_class = func->get_host_class()) {
        charly_to_string(io, host_class->get_name());
        io << ":";
      }

      charly_to_string(io, func->get_name());
      io << "#";
      io << func->get_minimum_argc();

      func->access_container_shared([&](Container::ContainerType* container) {
        for (auto& entry : *container) {
          io << " ";
          std::string key = SymbolTable::decode(entry.first);
          io << key;
          io << "=";
          charly_to_string(io, entry.second, trace);
        }
      });

      io << ">";

      trace.pop_back();
      break;
    }

    case kTypeCFunction: {
      trace.push_back(value);

      CFunction* func = charly_as_cfunction(value);
      io << "<CFunction ";
      charly_to_string(io, func->get_name());
      io << "#";
      io << func->get_argc();

      func->access_container_shared([&](Container::ContainerType* container) {
        for (auto entry : *container) {
          io << " ";
          std::string key = SymbolTable::decode(entry.first);
          io << key;
          io << "=";
          charly_to_string(io, entry.second, trace);
        }
      });

      io << ">";

      trace.pop_back();
      break;
    }

    case kTypeClass: {
      trace.push_back(value);

      Class* klass = charly_as_class(value);
      io << "<Class ";
      charly_to_string(io, klass->get_name());

      klass->access_container_shared([&](Container::ContainerType* container) {
        for (auto entry : *container) {
          io << " ";
          io << SymbolTable::decode(entry.first);
          io << "=";
          charly_to_string(io, entry.second, trace);
        }
      });

      io << ">";

      trace.pop_back();
      break;
    }

    case kTypeFrame: {
      io << "<Frame ";

      Frame* frame = charly_as_frame(value);
      Function* function = frame->get_function();
      if (Class* host_class = function->get_host_class()) {
        io << SymbolTable::decode(host_class->get_name());
        io << "::";
      }

      VALUE name = function->get_anonymous() ? SYM("<anonymous>") : function->get_name();
      io << SymbolTable::decode(name);

      io << ">";
      break;
    }

    default: {
      charly_debug_print(io, value, trace);
      break;
    }
  }
}

}  // namespace Charly
