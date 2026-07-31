extern decl print_u64_arg, u64, 0
extern decl print_char_arg, u8, 0
extern decl print_char_ptr_arg, u64, null
extern decl malloc_arg, u64, 0
extern decl malloc_ret, u64, 0
extern decl free_arg, u64, null
extern decl time_ret, u64, 0

extern decl err, u64, 0

extern proc print_u64
extern proc print_char
extern proc print_char_ptr
extern proc malloc
extern proc free
extern proc time


decl msg, u64, "Finished in "
decl msg2, u64, " nanoseconds\n"
decl err_msg, u64, "Couldn't read current time\n"

proc main:
    var ns_start, u64, 0    
    call time
    asg time_ret, ns_start
    gt err, 0, cond
    if cond:
        asg err_msg, print_char_ptr_arg
        call print_char_ptr
        ret


    var i, u64, 0
    var cond, u8, false
    loop:
        eq i, 1000000000, cond
        if cond:
            break
        add 1, i


    var ns_end, u64, 0
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
