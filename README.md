# json-parser-cpp

Version 1.0 of JSON Parser by Mikalai Varapai. Uses CMake as build tool and Catch2 for testing.

## Installation (Windows)
1. Clone the repository
2. From repository root directory, enter following commands:
   
   `mkdir build`
   
   `cd build`
   
   `cmake ..`
     
   `cmake --build .`
   
3. In directory `\build\src\Debug` there will be file `parser.exe`.

## Testing
1. Go to folder `\build\test\Debug`
2. Copy here the file `test1.json` from `\test`
3. Run `tests.exe`.

# Features

Application consists of 3 parts:
- JSON Reader
- JSON Interface
- CLI Expression Parsing

## JSON Reader

- Prints file and line of syntax error, as well as brief description.
- Builds a syntax tree, which can be used to access JSON fields whithin one session.
- Provides maximum tolerance with string literals - allows escape sequences and special characters.
  While it does successfully read and store such literals, other parts of the program do accept only limited
  range of object identifier names, making them inaccessible from the CLI.
- Gained performance through using interfaces to access JSON string.

## JSON Interface

- Ability to change current JSON object, making all JSON queries relative to that object.
- Can view contents of JSON Objects and Lists.
- Ability to specify recursive depth of syntax tree search to omit unnecessary details.

## CLI Expression Parsing

- Entering arbitrary expressions with basic arithmetic operators (`+`, `-`, `*`, `/`) - not limited to two operands.
- Basic functions `min`, `max` and `size`, as per specification.
- Support for entering numeric literals of `int` and `double`, and positive exponents.
