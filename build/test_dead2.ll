define i32 @test2() {
  %dead1 = add i32 5, 3           
  %dead2 = add i32 %dead1, 2      
  %live = add i32 10, 20          
  ret i32 %live
}
