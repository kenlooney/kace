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
  and an x86 plugin that encodes `nop` and short jumps
- Parsing of `mov`, `move`, and `copy` immediate expressions and registers
- Label recognition, offset recording, and short-jump fixups, plus a symbol
  table with save/load helpers
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
reported before the output file is opened. Near and far jumps are not yet
supported. Duplicate label definitions are not currently reported by the CLI.
The CLI does not yet save the symbol table.

For the jump example above, KASM produces `EB 01 90 EB FB`. Disassembling
that binary produces the following text:

```text
jmp 1
nop
jmp -5
```

The jump values are signed relative displacements; the original label names
are not stored in the raw binary. Disassembly uses canonical instruction names
such as `jmp`, even when the source used an alias such as `jump` or `goto`.
The original spelling is not preserved in the binary.

The CPU interface provides default implementations that report unsupported
operations, so plugins can override the encoders and decoder they support.
The x86 plugin also stores a 16-, 32-, or 64-bit mode, defaulting to 16-bit;
mode selection is not yet connected to the CLI.

The parser accepts immediate `mov` expressions,
and the x86 plugin can encode `mov al` with an 8-bit immediate, but that
encoding is not yet connected to the CLI's output path. Similarly, the x86
plugin can decode `nop`, `mov al, immediate`, and short jumps. To disassemble
an existing binary into a text file, run:

```powershell
.\bin\Debug\kasm.exe --disassemble -i example.bin
```

This writes `example.dism` next to the input. To choose a different name or
directory, pass `-o`; its extension is replaced with `.dism`. Decoded jumps
show their relative displacement, not the original label name, and are not yet
directly reassemblable. Unknown or incomplete instructions stop decoding with
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
symbol table persistence. The CLI tests also cover short-jump assembly and
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
- Near and far jumps, broader label resolution, and duplicate-label diagnostics
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
