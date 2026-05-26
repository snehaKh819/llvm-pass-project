define i32 @f(i32 %x) {
entry:
  %cond = icmp eq i32 %x, 0
  br i1 %cond, label %c, label %c

c:
  %phi = phi i32 [0, %entry], [1, %entry]
  ret i32 %phi
}
