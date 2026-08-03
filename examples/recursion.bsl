import <stdlib>

decl fact_arg, u8, 0
decl fact_res, u64, 1

// a program to calculate (fact_arg)! recursively

proc fact_rec:
    var cond, u8, false
    lt 1, fact_arg, cond
    if cond:
        mul fact_arg, fact_res
        dec fact_arg
        call fact_rec


proc main:
    asg 5, fact_arg
    call fact_rec

    asg fact_res, print_u64_arg
    call print_u64

    asg '\n', print_char_arg
    call print_char

