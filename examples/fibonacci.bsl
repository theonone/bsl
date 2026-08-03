import <stdlib>

// a program to calculate the Nth number of the Fibonacci sequence

var n, u64, 12 // input
decl fib_ret, u64, 0

proc empty:
    ret

proc fib:
    var cond, u8, 0

    lte n, 1, cond // cond = (n ≤ 1)
    if cond:
        asg n, fib_ret
        ret
    

    var a, u64, 0
    var b, u64, 1
    call empty 

    var next, u64, 1

    
    loop:
        gte 1, n, cond
        if cond:
            break

        asg b, next 
        add a, next
        asg b, a 
        asg next, b 

        dec n
    asg next, fib_ret

proc main:
    var test, u16, 0
    call fib
    asg fib_ret, print_u64_arg
    call print_u64
    asg '\n', print_char_arg
    call print_char
