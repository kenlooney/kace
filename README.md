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

- Byte-range semantic validation and raw binary output for byte definitions
- A CPU plugin interface with default unsupported-operation implementations,
  and an x86 plugin that encodes `nop`, short jumps, and 16-bit near and far jumps
- Parsing of `mov`, `move`, and `copy` immediate expressions and registers
- Label recognition, offset recording, and short- and near-jump fixups, plus a symbol
  table with save/load helpers
- Assembly and disassembly of 16-bit far jumps with segment and offset literals
- Extensible keyword and operator alias tables
- CTest-based automated tests

## Using KASM

For example, save this as `example.asm`:

```asm
start:
nop
define byte 3 sum 2
define_byte 7
nop
```

From a Windows Debug build, assemble it with:

```powershell
.\bin\Debug\kasm.exe --cpu=x86 -i example.asm -o example.bin
```

The output is the raw byte sequence `90 05 07 90`. The CPU defaults to
`x86`; use `-c` or `--cpu` to select a plugin. Run `kasm --help` for CLI
options. The plugin and shared libraries must be next to the executable.

Labels on their own lines are recorded while output is generated. A short
jump can refer to a label before or after the jump:

```asm
start:
jmp short done
nop
done:
jmp short start
```

The jump aliases `jump` and `goto` are also recognized. KASM reserves two
bytes per short jump, then resolves each target after recording labels. The
x86 plugin emits `EB` and a signed 8-bit displacement relative to the end of
the jump. Unknown labels and displacements outside -128 through 127 are
reported before the output file is opened. Duplicate label definitions are
reported as errors.

KASM also accepts `jmp near label` (and its `jump` and `goto` aliases). The
x86 plugin's current near form uses the 16-bit `E9` encoding: a three-byte
instruction with a signed 16-bit displacement, from -32768 through 32767,
relative to the end of the instruction.

KASM also assembles far jumps such as `jmp far 0x1234:0x5678`, with integer
literals for the segment and offset. The `jump` and `goto` aliases work here
too. In the x86 plugin's default 16-bit mode, both values must fit in 16 bits;
the example encodes as `EA 78 56 34 12`. Far jumps are disassembled with their
numeric segment and offset and do not require label fixups. Other modes are
not yet supported for far-jump encoding or decoding.
Each successful assembly writes a `.sym` symbol file next to the output binary,
using the same filename with its extension replaced (for example,
`example.bin` and `example.sym`). Keep these files together.

For the jump example above, KASM produces `EB 01 90 EB FB`. Disassembling
that binary with its matching symbol file produces the following text:

```text
start:
jmp short done
nop
done:
jmp short start
```

The symbol file supplies label names and offsets; the raw binary does not store
them. Disassembly uses canonical instruction names
such as `jmp`, even when the source used an alias such as `jump` or `goto`.
The original spelling is not preserved in the binary.

The CPU interface provides default implementations that report unsupported
operations, so plugins can override the encoders and decoder they support.
The x86 plugin also stores a 16-, 32-, or 64-bit mode, defaulting to 16-bit;
mode selection is not yet connected to the CLI.

The parser accepts immediate `mov` expressions,
and the x86 plugin can encode `mov al` with an 8-bit immediate, but that
encoding is not yet connected to the CLI's output path. Similarly, the x86
plugin can decode `nop`, `mov al, immediate`, short jumps, and 16-bit near and
far jumps. To disassemble
an existing binary into a text file, run:

```powershell
.\bin\Debug\kasm.exe --disassemble -i example.bin
```

This writes `example.dism` next to the input and automatically reads
`example.sym` if present. To choose a different output name or directory,
pass `-o`; its extension is replaced with `.dism`. Without a matching symbol
file, short and near jumps show numeric relative displacements and are not directly
reassemblable. With matching symbols, named targets are restored for short
and 16-bit near jumps. Keep the symbol file paired with its binary: KASM does not
verify that they match. Unknown or incomplete instructions stop decoding with
an error and do not replace an existing output file.

## Building

KACE requires:

- CMake 3.24 or newer
- A C++ compiler with C++17 support

Configure and build from the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

The executables and shared libraries are written to `bin/` (for a Visual
Studio Debug build, `bin/Debug/`). Existing presets are listed in
`CMakePresets.json`.

## Running Tests

Run the complete test suite with:

```powershell
ctest --test-dir build --output-on-failure -C Debug
```

The test suite covers source loading, diagnostics, lexing, expression and
statement parsing, CLI output, CPU plugins, x86 encoding and decoding, and
symbol table persistence. The CLI tests also cover short- and near-jump assembly and
disassembly, including failure cases. Use CTest's `-V` option to show output
from passing tests, or select a test in CMake Tools' Test Explorer.

## Repository Layout

```text
targets/
  kace/       Generic KACE library
  kasmcore/   KASM lexer and parser integration
  kasm/       KASM command-line executable
  plugins/x86/  x86 CPU plugin
tests/        Automated tests
examples/     Example assembly source files
```

## Roadmap

Planned development includes:

- Output for word, double-word, and quad-word data definitions
- Far-jump support in additional CPU modes and broader label resolution
- CPU mode selection and mode-aware encoding
- Connecting immediate encoding to the CLI and expanding disassembly
- More instructions, registers, and addressing modes
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
