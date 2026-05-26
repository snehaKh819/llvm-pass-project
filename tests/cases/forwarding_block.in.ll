; ModuleID = 'forwarding_block'
source_filename = "forwarding_block.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define void @f() {
entry:
  br label %block1

block1:
  br label %block2

block2:
  ret void
}
