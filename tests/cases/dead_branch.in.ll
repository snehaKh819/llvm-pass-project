; ModuleID = 'dead_branch'
source_filename = "dead_branch.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define void @f() {
entry:
  %cond = icmp eq i32 0, 0
  br i1 %cond, label %same, label %same

same:
  ret void
}
