add_llvm_pass_plugin(DeadCodeElimination
  DeadCodeElimination.cpp
  DEPENDS
  intrinsics_gen
  )

# Explicitly add LLVM include directories
target_include_directories(DeadCodeElimination PRIVATE ${LLVM_INCLUDE_DIRS})

# Add compiler definitions
target_compile_definitions(DeadCodeElimination PRIVATE ${LLVM_DEFINITIONS})

target_link_libraries(DeadCodeElimination PRIVATE 
  LLVMCore 
  LLVMSupport 
  LLVMTransformUtils
  )


  