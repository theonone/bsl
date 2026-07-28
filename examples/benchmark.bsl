import <stdlib>

decl i, u64, 0
decl cond, u8, false
decl ns_start, u64, 0
decl ns_end, u64, 0
decl msg, u64, "Finished in "
decl msg2, u64, " nanoseconds\n"
decl err_msg, u64, "Couldn't read current time\n"

proc main:

    call time
    asg time_ret, ns_start
    gt err, 0, cond
    if cond:
        asg err_msg, print_char_ptr_arg
        call print_char_ptr
        ret


    loop:
        eq i, 1000000000, cond
        if cond:
            break
        add 1, i


    call time
    asg time_ret, ns_end
    gt err, 0, cond
    if cond:
        asg err_msg, print_char_ptr_arg
        call print_char_ptr
        ret

    sub ns_start, ns_end

    asg msg, print_char_ptr_arg
    call print_char_ptr

    asg ns_end, print_u64_arg
    call print_u64

    asg msg2, print_char_ptr_arg
    call print_char_ptr
