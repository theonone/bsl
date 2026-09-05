# BSL

**BSL** (Bullshit Language) is a low-level programming language that uses an assembly-like syntax, explicit types, manual memory management, and a simple execution model. So far there's only one compiler for Linux that translates BSL programs into x86-64 assembly, which is then assembled with nasm and linked into an executable with gcc.

The project is still in early development. BSL is usable for toy programs, but its syntax, compiler internals, and standard library are still evolving, the produced code might contain errors.

# Getting Started
If you're not using Linux on the x86_64 architecture, you probably won't be able to run the code. MacOS or Free BSD might manage to, though.

If you are, then:
* Get the code onto your machine
* In the terminal, change into the bsl directory
* Run make to create the executable of the compiler
* Copy `bsl/compiler/libs/interfaces/stdlib.bsl` into `/usr/local/lib/bsl/bsl/` in order to make `import <stdlib>` work
* Run `nasm -f elf64 bsl/compiler/libs/impl/linux-x86_64/stdlib.asm -o /usr/local/lib/bsl/impl/linux-x86_64/o/stdlib.o` in order to create an object file of stdlib that can be imported with angle brackets
* Run `./bslc --help` to see how to work with it
* Try running `./bslc examples/io.bsl -o test_program`, then `./test_program`. If it works, you're all set!

The sequence of terminal commands should look like this:
* `cd bsl`
* `make`
* `sudo mv ./compiler/libs/interfaces/stdlib.bsl /usr/local/lib/bsl/bsl/`
* `sudo nasm -f elf64 ./compiler/libs/impl/linux-x86_64/stdlib.asm -o /usr/local/lib/bsl/impl/linux-x86_64/o/stdlib.o`

Test:
* `./bslc examples/io.bsl -o test_program`
* `./test_program`


## Examples

Examples of code can be found in [bsl/examples](https://github.com/theonone/bsl/tree/main/examples)

Examples include:

* Hello World.
* String reverse.
* Fibonacci calculation.
* Benchmark in two versions.
* A small calculator that adds two unsigned 64-bit integers.
* Factorial calculation via recursion.

## How BSLC (BSL Compiler) works

A BSL program is compiled into an executable through a few stages:

```text
BSL source
    ↓
Preprocessor
    ↓
Parser
    ↓
x86_64 translator
    ↓
NASM assembler
    ↓
GCC for linking
    ↓
Executable
```

The compiler is written in C++ and targets **x86-64 Linux**.

Compilation starts in bsl/bslc.cpp, run "bslc --help" for details

### Compilation model

BSL is currently compiled ahead of time. The compiler parses the source, performs the necessary checks, and generates assembly.

The generated assembly can then be assembled and linked using the normal Linux toolchain.

The exact build commands and compiler options are still subject to change as the project develops.

## Program structure

A BSL program consists of declarations, procedures, functions, and instructions.

### Procedures

A procedure is a named block of code that does not declare arguments or a return type.

```bsl
proc something:
    ret // returns back without a value

proc other: // ret is not necessary    

proc main:
    call something
```

Procedures are similar to functions in many languages, except for they can't have args or return values. Global variables or decls can be used to bypass the limitation.

### The entry point

The must contain an entry point - a procedure called "main"

```bsl
proc main:
    // ...
```

The compiler generates the necessary entry code for the executable to be ran prior to main.

## Variables and declarations

BSL currently has two types of variables:

### Declarations

`decl name, type, value` creates a declaration.

```bsl
decl counter, u64, 0
decl message, u64, "Hello, World!\n"

proc whatever:
    ...
```

Declarations are stored in the program's data section.

They can be used for persistent data, strings, and communication between procedures. They function like global variables in other languages.

### Variables

`var name, type, value` creates a variable.

```bsl
proc main:
    var counter, u64, 0
    var condition, u8, false
```

Variables live on the stack if they aren't inside the global scope. Otherwise they're equivalent to decls.

The compiler tracks their stack positions and generates the required stack operations, automatically deallocates variables when their parent scope is exited.

### Scope

BSL has explicit and automatic scopes.

```bsl
proc something:
    if condition:
        var x, u64, 10
        // x exists here
    // but not here
```

Variables are removed from the active scope when execution leaves that scope.

The compiler also handles cleanup when leaving nested scopes through control-flow instructions such as `break`, `continue`, and `ret`.

## Types

BSL currently has a small set of types.

| Type  | Meaning                 |
| ----- | ----------------------- |
| `u8`  | 8-bit unsigned value  |
| `u16` | 16-bit unsigned value |
| `u32` | 32-bit unsigned value |
| `u64` | 64-bit unsigned value |
| `i8`  | 8-bit signed value    |
| `i16` | 16-bit signed value   |
| `i32` | 32-bit signed value   |
| `i64` | 64-bit signed value   |

Some operations, mostly arithmetic, require both values to be of the same signedness.

BSL does not currently support casts, but you can use assignments instead.

## Basic Instructions

BSL uses one-command-per-line instructions.

### Assignment

```bsl
asg 10, x
asg x, y
```

Assignment copies the value of the first operand into the second operand. For many operations, order is inverse to assembly for a simple reason: when you read a line like "asg 10, x", you can read it like "assign 10 to x", which is fairly intuitive

### Arithmetic

```bsl
add a, b // add a to b
sub a, b
mul a, b 
div a, b
```

The instructions transform the second operands using the first. For example, "div a, b" in C would be "b /= a;" or "b = b/a;"

### Increment and decrement

```bsl
inc counter
dec counter
```

### Comparisons

```bsl
eq a, b, condition // condition = (a==b)
gt a, b, condition // condition = (a > b), because gt means "greater than". condition = "a greater than b"
lt a, b, condition // condition = (a < b)
```

The comparison result is written to the destination as 0 or 1.

### Boolean operations

```bsl
not condition
or a, b
and a, b
xor a, b
```

The result is written into the second operand.

### Control flow

```bsl
if condition:
    // ...

loop:
    // ...

break
continue
```

BSL supports conditional execution, loops, and loop control.

### Procedure calls

```bsl
call get_uint64
call print_u64
```

The current standard-library interface uses global argument and return variables.

For example:

```bsl
asg value, print_u64_arg
call print_u64

asg 10, malloc_arg
call malloc
asg malloc_ret, ptr
```

### Return

```bsl
ret
```

Returns from the current procedure.

### Program exit

```bsl
exit
```

Terminates the program. Might be replaced with a stdlib procedure in the future

## Memory

BSL supports explicit memory operations.

### Allocation

```bsl
import <stdlib>

proc main:
    asg ..., malloc_arg
    call malloc
    asg malloc_ret, ...
```

Malloc is declared in stdlib, uses libc's implementation under the hood.
### Load and store

```bsl
store value, address // in C: address* = value;
load address, value // value = *address;
addr src, dest // dest = &src;
```

These instructions allow programs to work with memory.

### Freeing memory

```bsl
import <stdlib>

proc main:
    asg ptr, free_arg
    call free
```

Just like malloc, it's defined inside stdlib, and uses libc.


## Strings

String literals are supported.

```bsl
decl message, u64, "Hello, World!\n"
```

In the example, `message` will store the address of the first byte ('H'), the string will be automatically appended with '\0'

Currently all string literals must only be used in decls.

## Standard library

BSL can call some external C functions through libc.

The standard library is currently small and includes basic I/O and memory-management functionality.

### Current examples

```bsl
call get_char
call print_char
call print_char_ptr
call print_u64
call malloc
call free
```

The standard library is implemented as BSL procedures and external libc functions where appropriate.

## Imports and linking

```bsl
import <stdlib>
import "path/to/something.bsl"
link <stdlib.o>
link "/home/.../obj.o"
```

The `import` instruction, like in C, during preprocessing is simply replaced with the contents of the file.
`link` asks the linker to link the results with the requested file.

If the target is specified with quote marks, the path will be used as is. 


`import <library>` is equivalent to `import "/usr/local/lib/bsl/bsl/library.bsl"`
In angle-bracket imports, .bsl extension of the target file is assumed automatically
`link <library.o>` is equivalent to `link "/usr/local/lib/bsl/impl/{os}-{arch}/o/library.o"` where {os} is your OS (can only be "linux" as for now), {arch} is your CPU architecture (x86_64)

## What exists today

BSL currently has:

* A working compiler written in C++ for x86_64 Linux.
* Types with explicit sizes and signedness.
* Global declarations.
* Stack variables.
* Indent-based scoping.
* Basic instructions.
* Conditional execution.
* Loops.
* Procedures.
* Memory management.
* String literals.
* Import support.
* Linking with external assembly.
* A basic standard library.

## What is not implemented yet

The following features are not currently available or incomplete:

* Floating-point types and operations.
* Functions.
* Structs and user-defined types.
* A complete type-conversion system.
* An actual code optimizer.
* Cross-platform code generation.

## Current limitations

BSL is still a work in progress.

The language syntax may change without backward compatibility. Compiler internals are also changing rapidly as new features are added.

Programs should be considered experimental rather than production-ready.

## License

GNU Public License v3
