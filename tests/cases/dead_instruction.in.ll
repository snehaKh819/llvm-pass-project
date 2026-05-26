; ModuleID = 'dead_instruction'
source_filename = "dead_instruction.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define i32 @main() {
entry:
  %a = add i32 1, 2
  %b = add i32 3, 4
  %c = add i32 %a, 5
  ret i32 %c
}
