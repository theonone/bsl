import <stdlib>

// a simple program to reverse strings (str)

decl str, u64, "Hello World!" // input
decl intro, u64, "Initial string:\n"
decl rev, u64, "Reversed:\n"
decl rs_arg, u64, null
decl sl_arg, u64, null
decl sl_ret, u64, 0

proc str_len:
    asg 0, sl_ret
    var cond, u8, false
    var char, u8, 0
    loop:
        load sl_arg, char
        eq char, 0, cond  
        if cond:
            break
        inc sl_arg
        inc sl_ret


// reverses all characters up until \0
proc reverse_str: 
    asg rs_arg, sl_arg
    call str_len

    var ptr, u64, null
    var ptr2, u64, null
    asg rs_arg, ptr
    asg ptr, ptr2
    add sl_ret, ptr2
    sub 1, ptr2
    div 2, sl_ret

    var i, u64, 0
    var cond, u8, false

    loop:
        var char, u8, 0
        var char2, u8, 0

        gte i, sl_ret, cond // i >= sl_ret
        if cond:
            break

        load ptr, char
        load ptr2, char2
        store char, ptr2
        store char2, ptr

        inc ptr
        dec ptr2
        inc i


proc main:
    asg intro, print_char_ptr_arg
    call print_char_ptr

    asg str, print_char_ptr_arg
    call print_char_ptr

    asg '\n', print_char_arg
    call print_char


    asg rev, print_char_ptr_arg
    call print_char_ptr

    asg str, rs_arg
    call reverse_str

    asg str, print_char_ptr_arg
    call print_char_ptr

    asg '\n', print_char_arg
    call print_char