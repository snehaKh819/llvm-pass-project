define i32 @test1() {
  %dead1 = add i32 5, 3           
  %live = add i32 10, 20          
  ret i32 %live
}
