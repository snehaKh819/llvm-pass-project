define void @f() {
entry:
  %cond = icmp eq i32 0, 0
  br label %same

same:
  ret void
}
