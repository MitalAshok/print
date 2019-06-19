print
=====

Header only Python like print function for C++ (C++11 compatible)

Just include `include/print.h`.

Example
-----

```c++
#include <stringstream>
#include <iosmanip>
#include "print.h"
  
int main() {
    print("Hello, world!");
    print("It's as easy as", 3.14159265);
  
    print("Bu", "t I don't want a space", sep="");
  
    print("And I don't want a", end=" ");
    print("newline", end=" ");
    print("until now!");
  
    std::stringstream ss;
    print("We can use other streams", file=ss);
    // (Or anything with an `operator<<` to output)
  
    // `flush` calls `file.flush()`, where `file` defaults to `std::cout`
    print("And we can flush streams too", flush=true);
    print("Or not", flush=false);  // Or a variable bool
    print("Or we can flush again", flush); // `flush` means `flush=true`
  
    // `print_nothing` suppresses the space
    print(
        "And we can stop se", print_nothing, "p insertions",
        "in specific", "places"
    );
  
    // Without the `print_nothing` there would be a space before and after `std::boolalpha`
    print(
        "Is this good for stream modifiers that don't print anything?",
        print_nothing, std::boolalpha,
        true
    );
  
    print(
        "print_nothing is also a more efficient empty string\n",
        end=print_nothing
    );
  
    print("Which is why it's the default if you write nothing\n", end); 
  
    raw_print(
        "raw_print is just print where `end` defaults to `print_nothing` "
        "and `sep` defualts to `print_nothing` too.\n"
    );
  
    print_no_end("print_no_end is the same, except `sep` is still `' '` (space)");
}
```

For more detail, see the file itself.

Tested on g++-8, clang++-7 and MSVC++14.1.
