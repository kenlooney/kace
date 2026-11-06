# KACE

**Kens Assembly and Compiler Environment**

KACE is a portable framework for building assemblers and compiler front ends.
It provides reusable source loading, lexing, alias tables, expression parsing,
abstract syntax trees, statement parsing, and semantic validation.

KASM is the first assembler being developed on top of KACE. Its syntax is
designed to be readable and approachable while still providing familiar
assembly concepts.

> **Project status:** KACE and KASM are under active development. The current
> release is a development snapshot and should not yet be considered a stable
> assembler toolchain.

## Current Features

- Portable CMake-based build
- Windows and Linux build support
- Decimal, binary, hexadecimal, and octal integer literals
- Operator aliases such as:
  - `sum` for `+`
  - `take` for `-`
  - `mul` for `*`
  - `div` for `/`
  - `mod` for `%`
- Expression parsing with operator precedence and grouping
- AST-backed expression evaluation
- Multiline source parsing with line-aware diagnostics
- Named data definitions, including:

  ```asm
  define byte 3 sum 2
  define byte 12 mul 4
  define byte ((8 take 3) mul 2)
  ```

- Byte-range semantic validation
- Extensible keyword, operator, and alias tables
- CTest-based automated tests

## Example Output

The current multiline parser evaluates the example above as:

```text
line 1: Checked byte: 5
line 2: Checked byte: 48
line 3: Checked byte: 10
parsed 3 statements
```

## Building

KACE requires:

- CMake 3.24 or newer
- A C++ compiler with C++17 support

Configure and build from the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

The executables and shared libraries are written to `bin/`.

## Running Tests

Run the complete test suite with:

```powershell
ctest --test-dir build --output-on-failure -C Debug
```

The test suite currently covers source loading, diagnostics, token display,
integer parsing, expression parsing, and multiline statement parsing.

## Repository Layout

```text
targets/
  kace/       Generic KACE library
  kasmcore/   KASM lexer and parser integration
  kasm/       KASM command-line executable
tests/        Automated tests
examples/     Example assembly source files
```

## Roadmap

Planned development includes:

- Word, double-word, and quad-word data definitions
- Binary output generation
- Instruction and operand parsing
- Register and addressing-mode support
- A complete KASM command-line workflow
- Stable releases and packaged distributions

## Releases

| Type | Description |
|------|-------------|
| Development snapshot | The current development version. It may change frequently and may contain incomplete features. |
| Stable | A tested release intended for regular use. No stable release is available yet. |

## License

KACE is distributed under the GNU General Public License, version 3 or later.
See [LICENSE](LICENSE) for the full license text.
