extern decl print_u64_arg, u64, 0
extern decl print_char_arg, u8, 0
extern decl print_char_ptr_arg, u64, null
extern decl malloc_arg, u64, 0
extern decl malloc_ret, u64, 0

extern decl free_arg, u64, null

extern proc print_u64
extern proc print_char
extern proc print_char_ptr
extern proc malloc
extern proc free


link <stdlib.o>