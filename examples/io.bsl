import <stdlib>

// a program that reads two numbers from stdin and outputs the sum

decl get_uint64_ret, u64, 0
decl str1, u64, "Let's add two numbers!\n"
decl str2, u64, "Enter number 1:\n"
decl str3, u64, "Enter number 2:\n"
decl str4, u64, "Result:\n"
decl err1_str, u64, "Overflow! Exiting...\n"
decl err2_str, u64, "Only numeric characters allowed! Exiting...\n"


proc get_uint64:
    var num, u64, 0
    var i, u8, 0
    var last, u64, 0
    var ptr, u64, null
    var char, u8, 0
    var cond, u8, false
    var curr, u64, null

    asg 20, malloc_arg // uint64 max is 20 digits long (18446744073709551615)
    call malloc
    asg malloc_ret, ptr
    asg ptr, curr

    loop:
        var bool2, u8, false

        call get_char
        asg get_char_ret, char 
        eq char, 0, cond

        eq char, '\n', bool2
        or bool2, cond

        eq char, ' ', bool2
        or bool2, cond

        if cond:
            break
        
        gt char, '9', cond
        lt char, '0', bool2
        or bool2, cond

        if cond:
            asg err2_str, print_char_ptr_arg
            call print_char_ptr
            exit
        
        // 21st char can be a terminator, but not a number
        eq i, 20, cond
        if cond:
            asg err1_str, print_char_ptr_arg
            call print_char_ptr
            exit

        store char, curr
        inc curr
        inc i
    
    var len, u8, 0
    dec curr
    asg i, len
    asg 0, i
    
    loop:
        eq i, len, cond
        if cond:
            break
        
        var temp, u64, 0
        var pwr, u8, 0
        load curr, char
        asg char, temp
        asg i, pwr
        sub 48, temp

        loop: 
            eq pwr, 0, cond
            if cond:
                break
            mul 10, temp
            dec pwr

        add temp, num
        inc i
        dec curr
        lt num, last, cond
        if cond:
            asg err1_str, print_char_ptr_arg
            call print_char_ptr
            exit
        asg num, last
    asg num, get_uint64_ret

    asg ptr, free_arg
    call free
    ret



proc main:
    asg str1, print_char_ptr_arg
    call print_char_ptr
    
    asg str2, print_char_ptr_arg
    call print_char_ptr

    var num1, u64, 0
    call get_uint64
    asg get_uint64_ret, num1

    asg str3, print_char_ptr_arg
    call print_char_ptr

    call get_uint64

    asg str4, print_char_ptr_arg
    call print_char_ptr

    add num1, get_uint64_ret
    var cond, u8, false
    lt get_uint64_ret, num1, cond
    if cond:
        asg err1_str, print_char_ptr_arg
        call print_char_ptr
        exit
    asg get_uint64_ret, print_u64_arg
    call print_u64

    asg '\n', print_char_arg
    call print_char