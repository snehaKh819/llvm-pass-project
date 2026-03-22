add_llvm_pass_plugin(DeadCodeElimination
  DeadCodeElimination.cpp
  DEPENDS
  intrinsics_gen
  )
target_include_directories(DeadCodeElimination PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(DeadCodeElimination PRIVATE LLVMCore LLVMSupport LLVMTransformUtils)