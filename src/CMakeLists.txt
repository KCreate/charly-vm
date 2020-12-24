file(GLOB charlycore_SRC
  "charly/core/*.cpp"
  "charly/utils/*.cpp"
)

add_library(libcharly ${charlycore_SRC})
target_include_directories(libcharly PRIVATE "./")
SET_TARGET_PROPERTIES(libcharly PROPERTIES PREFIX "")
