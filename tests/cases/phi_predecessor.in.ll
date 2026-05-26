; ModuleID = 'phi_predecessor'
source_filename = "phi_predecessor.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define i32 @f(i32 %x) {
entry:
  %cond = icmp eq i32 %x, 0
  br i1 %cond, label %a, label %b

a:
  br label %c

b:
  br label %c

c:
  %phi = phi i32 [0, %a], [1, %b]
  ret i32 %phi
}
