file(GLOB charlycore_SRC
  "charly/core/**/*.cpp"
  "charly/utils/**/*.cpp"
)


add_library(libcharly ${charlycore_SRC})
target_include_directories(libcharly PRIVATE "./")
target_compile_features(libcharly PRIVATE cxx_std_17)
set_target_properties(libcharly PROPERTIES PREFIX "")
