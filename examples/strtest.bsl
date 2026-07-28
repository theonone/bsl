import <stdlib>

// a simple program to reverse strings (str)

decl str, u64, "Hello World!" // input

decl intro, u64, "Initial string:\n"
decl rev, u64, "Reversed:\n"
decl rs_arg, u64, null
decl sl_arg, u64, null
decl sl_cnt, u64, 0
decl cond, u8, false
decl ptr, u64, null
decl ptr2, u64, null
decl char, u8, 0
decl char2, u8, 0
decl i, u64, 0


proc str_len:
    asg 0, sl_cnt
    asg sl_arg, ptr
    loop:
        load ptr, char
        eq char, 0, cond  
        if cond:
            break
        add 1, ptr
        add 1, sl_cnt


// reverses all characters up until \0
proc reverse_str: 
    asg rs_arg, sl_arg
    call str_len
    asg rs_arg, ptr
    asg ptr, ptr2
    add sl_cnt, ptr2
    sub 1, ptr2
    div 2, sl_cnt
    loop:
        gte i, sl_cnt, cond // i >= sl_cnt
        if cond:
            break

        load ptr, char
        load ptr2, char2
        store char, ptr2
        store char2, ptr

        add 1, ptr
        sub 1, ptr2
        add 1, i


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