add_library(DeadCodeElimination MODULE
  DeadCodeElimination.cpp
)

target_include_directories(DeadCodeElimination PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(DeadCodeElimination PRIVATE ${LLVM_DEFINITIONS})

set_target_properties(DeadCodeElimination PROPERTIES
  PREFIX ""
)