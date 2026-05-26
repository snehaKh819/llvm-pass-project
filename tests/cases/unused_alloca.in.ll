; ModuleID = 'unused_alloca'
source_filename = "unused_alloca.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define void @f() {
entry:
  %p = alloca i32, align 4
  store i32 42, i32* %p, align 4
  ret void
}
