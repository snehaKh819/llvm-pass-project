llvm_map_components_to_libnames(LLVM_LIBS
  core
  support
  analysis
  scalaropts
  transformutils
  ipo
  passes
  irreader
)

# Build as plugin
add_library(DeadCodeElimination MODULE
  DeadCodeElimination.cpp
)

target_include_directories(DeadCodeElimination PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(DeadCodeElimination PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(DeadCodeElimination PRIVATE ${LLVM_LIBS})

set_target_properties(DeadCodeElimination PROPERTIES
  PREFIX ""
)

# Build as standalone tool (avoids plugin loading issues)
add_executable(dce_tool
  dce_tool.cpp
  DeadCodeElimination.cpp
)

target_include_directories(dce_tool PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(dce_tool PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(dce_tool PRIVATE ${LLVM_LIBS})