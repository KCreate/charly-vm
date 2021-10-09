#include "termcolor.h"

namespace termcolor::_internal {
  static int colorize_index = ((std::cout << "xalloc" << std::endl), std::ios_base::xalloc());
}
