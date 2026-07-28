import <stdlib>

// a program to calculate the Nth number of the Fibonacci sequence

decl n, u64, 12 // input

decl cond, u8, false
decl a, u64, 0
decl b, u64, 1
decl next, u64, 0
decl i, u64, 2

proc fib:
    lte n, 1, cond // cond = (n ≤ 1)
    if cond:
        asg n, next
        ret
    
    loop:
        gt i, n, cond
        if cond:
            break

        asg b, next 
        add a, next
        asg b, a 
        asg next, b 

        add 1, i

proc main:
    call fib
    asg next, print_u64_arg
    call print_u64
