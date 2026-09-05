import <stdlib>

decl msg, u64, "Hello World!"

proc main:
    asg msg, print_char_ptr_arg
    call print_char_ptr

    asg '\n', print_char_arg
    call print_char