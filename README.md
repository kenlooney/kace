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
- Source spans on expression nodes and caret diagnostics for supported arithmetic errors
- Byte definitions with `byte`, `definebyte`, or `db`, including:

  ```asm
  byte 3 sum 2
  definebyte 12 mul 4
  db ((8 take 3) mul 2)
  ```

- Byte-range semantic validation and raw binary output for byte definitions
- Repeated byte definitions and position-based padding with `times`, `fill`, `repeat`, or `dup`
- Source-file inclusion with nested includes and cycle detection
- Assembly-time output-position assertions with `#assertposition`
- CPU plugins with assembler and optional disassembler contracts,
  and an x86 plugin that encodes `nop`, software interrupts, `hlt`, short jumps,
  mode-aware near jumps, and 16-bit far jumps
- Architecture-neutral register files and x86 register families with named,
  overlapping views and encoding IDs
- An inheritable statement interface and named statement kinds, with KASM's
  statement data stored in its own derived class
- x86 plugin API encoding and decoding of supported register-immediate moves,
  including mode-aware operand sizes
- x86 plugin API encoding and decoding of register-to-register moves in 16-,
  32-, and 64-bit modes for the first eight register groups
- x86 memory-to-register loads in 16-, 32-, and 64-bit modes, with KASM source support
- Parsing of `mov`, `move`, and `copy` immediate expressions and registers
- Label recognition, offset recording, and short- and near-jump fixups, plus a symbol
  table with save/load helpers
- Symbol definition locations and byte ranges mapped to source statements
- Short conditional jumps with label fixups and readable condition aliases
- Assembly and disassembly of 16-bit far jumps with segment and offset literals
- Extensible keyword and operator alias tables
- CTest-based automated tests

## Using KASM

For example, save this as `example.asm`:

```asm
start:
nop
byte 3 sum 2
definebyte 7
nop
```

From a Windows Debug build, assemble it with:

```powershell
.\bin\Debug\kasm.exe --cpu=x86 -i example.asm -o example.bin
```

The output is the raw byte sequence `90 05 07 90`. The CPU defaults to
`x86`; use `-c` or `--cpu` to select a plugin. Run `kasm --help` for CLI
options. The plugin and shared libraries must be next to the executable.

### Software interrupts and halt

KASM assembles and disassembles the x86 software interrupt and halt instructions:

```asm
int 0x21       ; also spelled interrupt 0x21
halt           ; also spelled hlt
```

These instructions emit `CD 21 F4`. The interrupt vector must be an integer
literal from 0 through 255; its meaning depends on the execution environment.
Disassembly uses the canonical names `int` and `hlt`, and displays the vector
in decimal. The `hlt` instruction requires appropriate CPU privileges at
runtime and emits no operand.

### Memory loads

KASM can load from a register-based memory address into a register:

```asm
code16
mov ax, [bx]     ; 8B 07
code32
mov eax, [ebx]   ; 8B 03
code64
mov rax, [rbx]   ; 48 8B 03
```

The first form supports `[bx]`, `[si]`, and `[di]`. The 32- and 64-bit
forms support the first eight register groups as destinations and simple
bases other than `[esp]`/`[rsp]` and `[ebp]`/`[rbp]`. Indexed addresses,
displacements, extended registers, and memory stores are not yet supported.
The x86 decoder uses mode metadata from the matching symbol file when available;
otherwise it uses the selected CPU mode. Raw binaries do not record mode changes.

### Expression diagnostics

KASM can show the source line and a caret when an expression has an arithmetic
error. For example, `byte 10 / 0` produces:

```text
example.asm:1: Division by zero
byte 10 / 0
          ^
```

Division and modulo by zero point to the divisor. Addition and multiplication
overflow and subtraction that would produce a negative value point to the start
of the expression. Other errors do not yet all supply a source span. The KACE
expression evaluator can return these spans to other consumers; KASM formats
the source line and caret in its error message.

## Supported directives

| Directive | Purpose | When handled |
|-----------|---------|--------------|
| `byte`, `definebyte`, `db` | Emit one byte from an expression. | Assembly |
| `times`, `fill`, `repeat`, `dup` | Repeat a byte definition using a count expression. | Assembly |
| `#include` | Insert statements from another source file. | Source expansion |
| `#assertposition` | Check the current output offset without emitting bytes. | Assembly |

### Byte definitions

Use `byte`, `definebyte`, or `db` followed by an expression to emit one byte;
the evaluated value must be between 0 and 255. The older `define byte` form
is also currently accepted.

### Repeating bytes and padding

Use `times`, `fill`, `repeat`, or `dup` with a count expression, a byte
directive (`byte`, `definebyte`, or `db`), and a value expression:

```asm
fill 4 byte 0
repeat 3 definebyte 2 sum 1
```

These lines emit `00 00 00 00 03 03 03`. The count can be zero and is
limited to 1,000,000 repetitions per directive. The value must fit in one
byte; repeating wider data types is not yet supported.

For padding, `currentposition` (or `$`) is the output offset at the start
of the directive. `sectionstart` (or `$$`) is zero in the current flat output;
sections are not yet implemented. The count is evaluated when the directive
is emitted:

```asm
nop
fill 10 - (currentposition - sectionstart) byte 0
byte 0x55
```

The `nop` emits one byte, the `fill` adds nine zero bytes, and the final
byte brings the total to eleven. If the output is already past the requested
position, the subtraction reports an error instead of wrapping.

### Including source files

Use `#include` to insert another source file at the directive's position:

```asm
byte 1
#include "data/extra.asm"
byte 4
```

If `data/extra.asm` contains `byte 2` and `byte 3`, the output is
`01 02 03 04`. Paths must be non-empty and quoted with single or double
quotes. Relative paths are resolved from the directory of the file containing
the directive, so included files can include files of their own.

Included statements share the same output and label table as the main source.
Missing files, include cycles, and nesting deeper than 128 active source files
(including the main file) produce errors. The same file can be included again
after a previous inclusion has finished.

### Checking the output position

Use `#assertposition` followed by a single nonnegative integer literal to
check the current byte offset during assembly:

```asm
byte 1
byte 2
#assertposition 2
byte 3
```

The assertion emits no bytes; this example produces `01 02 03`. If the
current offset differs from the expected position, assembly stops with an
error showing both values. The position is measured from the beginning of
the output, including bytes emitted by included files. Decimal and other
supported integer literal forms can be used; expressions are not yet accepted
for this directive.

## Jumps and disassembly

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

### Short conditional jumps

KASM supports four short conditional jumps, with aliases for readable source:

| Instruction | Alias | Condition | Opcode |
|-------------|-------|-----------|--------|
| `jz` | `ifequal` | Zero flag is set | `74` |
| `jnz` | `ifnotequal` | Zero flag is clear | `75` |
| `jc` | `ifcarry` | Carry flag is set | `72` |
| `jnc` | `ifnotcarry` | Carry flag is clear | `73` |

The instructions test CPU flags; they don't compare values themselves. When a
preceding comparison sets the zero flag, `ifequal` and `ifnotequal` can be used
to branch on its result. For example:

```asm
start:
ifequal done
nop
done:
ifnotequal start
```

This produces `74 01 90 75 FB`. The assembler resolves forward and backward
labels, and each displacement must fit in -128 through 127 bytes from the end
of the two-byte instruction. These are short forms in all supported modes;
near conditional forms are not yet supported.

Disassembly uses the canonical names and prints numeric displacements, such as
`jz 1` and `jnz -5`. Conditional targets are not yet restored from symbol
files, and the output's numeric conditional targets are not currently accepted
by the label-based parser.

### Near and far jumps

KASM also accepts `jmp near label` (and its `jump` and `goto` aliases). In the
CLI's default 16-bit x86 mode, the `E9` near jump is three bytes with a signed
16-bit displacement, from -32768 through 32767, relative to the instruction's
end. Through the x86 plugin's C++ API, 32- and 64-bit modes use a five-byte
`E9` form with a signed 32-bit displacement. Near-jump label fixups use the
mode selected for each jump. Source statements `code16`, `code32`, and
`code64` change the mode used for subsequent near-jump assembly. These
statements do not emit bytes. The symbol file records the x86 mode for emitted
byte ranges. Disassembly uses that metadata and emits mode changes where needed;
without it, the plugin's selected mode is used. Raw binaries do not record mode
changes.

KASM also assembles far jumps such as `jmp far 0x1234:0x5678`, with integer
literals for the segment and offset. The `jump` and `goto` aliases work here
too. In the x86 plugin's default 16-bit mode, both values must fit in 16 bits;
the example encodes as `EA 78 56 34 12`. Far jumps are disassembled with their
numeric segment and offset and do not require label fixups. Other modes are
not yet supported for far-jump encoding or decoding.
Each successful assembly writes a `.sym` symbol file next to the output binary,
using the same filename with its extension replaced (for example,
`example.bin` and `example.sym`). It stores label offsets and definition
locations, as well as emitted byte ranges and their source file, line, and
column. Duplicate-label errors identify both definitions, including when a
label was defined in an included file. Keep the `.sym` file with its binary.

The current versioned `.sym` format records both symbols and byte ranges.
KASM can also read the older symbol-only format; older KASM releases cannot
read the new format. Source locations in older files are unavailable.

For the jump example above, KASM produces `EB 01 90 EB FB`. Disassembling
that binary with its matching symbol file produces output like this (the
timestamp and source path depend on the run):

```text
; Generated by the KASM disassembler
; Generated at: 2026-11-12 12:00:00 UTC

start:
; source: example.asm:2:1
jmp short done
; source: example.asm:3:1
nop
done:
; source: example.asm:5:1
jmp short start
```

The symbol file supplies labels and source locations; the raw binary does not
store them. Disassembly uses canonical instruction names
such as `jmp`, even when the source used an alias such as `jump` or `goto`.
The original spelling is not preserved in the binary.

The CPU interface exposes a plugin-provided assembler and an optional
disassembler. The x86 plugin keeps its instruction encoders, decoders, and
jump fixups inside the plugin. The x86 plugin also stores a 16-, 32-, or
64-bit mode, defaulting to 16-bit.

### Statements and parser implementations

KACE's `Statement` is a base class with a name and a collection of named
`StatementKind` objects. Kinds are held by `std::unique_ptr`, allowing a
derived kind to retain its own data without being sliced. A statement can
have multiple kinds, and `has_kind()` checks their names.

KASM defines `KasmStatement`, which derives from `Statement` and holds its
own expression, register, directive, jump, mode, and source-location data.
The parser creates these derived objects and passes them through the shared
assembler contract as `std::unique_ptr<Statement>`. Include expansion keeps
the same ownership model. The x86 assembler checks that each statement is a
`KasmStatement` before accessing KASM-specific fields; it currently depends
on `kasmcore` and is not a language-independent statement consumer.

Another language implementation can define a different `Statement` subclass
and have its parser construct it. A concrete subclass must override the base
class's pure virtual `parse_statement()` function. Currently,
`KasmStatement::parse_statement()` is a placeholder that returns `false`;
`KasmParser::parse()` still performs KASM's actual parsing. How that work
should be divided is an open part of the design. Assemblers consuming other
statement types need to handle those types explicitly, and plugins must be
rebuilt against the matching framework interface.

### Registers and moves

The framework exposes a CPU's `RegisterFile` through the `Cpu` interface.
Plugins can define architecture-specific register information without putting
x86 encoding rules into the generic register types. The x86 plugin groups
overlapping names such as `rax`, `eax`, `ax`, `al`, and `ah`; each alias
describes its width and position within the group. Register encoding IDs let
the encoder distinguish, for example, `al` from `ah`.

Through the x86 plugin's C++ encoder API, immediate moves support the eight
8-bit registers `al`, `cl`, `dl`, `bl`, `ah`, `ch`, `dh`, and `bh`, and
the first eight 16- and 32-bit general-purpose registers. The 8-bit form uses
opcode `B0` plus the register ID and one immediate byte. The 16- and 32-bit
forms use `B8` plus the register ID and a little-endian immediate; an operand
size prefix is emitted when the register width differs from the mode's default.
For example, in 16-bit mode, `mov cx, 0x1234` encodes as `B9 34 12`, and
`mov ah, 0x42` as `B4 42`. In 32-bit mode, `mov cx, 0x1234` uses
`66 B9 34 12`.

The decoder recognizes these 8-bit moves and `B8`–`BF` immediate moves for
the first eight 16-, 32-, and 64-bit register groups. It interprets operand
size according to the selected mode, the `66` override, and `48` for the
64-bit form. For example, in 64-bit mode `48 B8 78 56 34 12 00 00 00 00`
decodes to `mov rax, 305419896`. Other REX prefixes and extended registers
are not yet supported by this decoder. The immediate encoder also supports
64-bit primary registers such as `rax` in 64-bit mode for the first eight
register groups.

The x86 assembler also calls this encoder for parsed immediate `mov`
statements. For example, `mov cx, 0x1234` in the default 16-bit mode produces
`B9 34 12` through the KASM CLI.

The x86 plugin's encoder and decoder also support register-to-register moves
for the first eight general-purpose register groups. The current encoder
supports the native-width form in each mode:

| CPU mode | Example | Encoded bytes |
|----------|---------|---------------|
| 16-bit | `mov ax, bx` | `89 D8` |
| 32-bit | `mov eax, ebx` | `89 D8` |
| 64-bit | `mov rax, rbx` | `48 89 D8` |

The `89` opcode is followed by a ModR/M byte that identifies the source and
destination; the `48` prefix selects a 64-bit operand. In 64-bit mode, the
decoder also reads unprefixed `89 D8` as `mov eax, ebx`. The register-move
encoder is not yet connected to KASM source parsing or the assembler, so the
examples above describe the plugin API and decoder, not CLI assembly. Memory
loads are described above; extended registers (`r8`–`r15`), and additional operand-size and REX
prefix forms remain future work for register moves.

The x86 plugin can decode `nop`, `int`, `hlt`, supported register-immediate, register-to-register, and memory-load moves, short
unconditional and conditional jumps, mode-aware near jumps, and 16-bit far jumps.
To disassemble an existing binary into a text
file, run:

```powershell
.\bin\Debug\kasm.exe --disassemble -i example.bin
```

This writes `example.dism` next to the input and automatically reads
`example.sym` if present. The generated file starts with a comment identifying
KASM and the generation time in UTC; matching source ranges are shown as
comments when available. A source comment is currently emitted when an
instruction's byte offset and length exactly match a saved range. A larger
data or padding range may not receive a comment, and source metadata does not
identify whether bytes are instructions or data. To choose a different output name or directory,
pass `-o`; its extension is replaced with `.dism`. Without a matching symbol
file, short and near jumps show numeric relative displacements and are not directly
reassemblable. With matching symbols, named targets are restored for short
and near jumps. Keep the symbol file paired with its binary: KASM does not
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
symbol table persistence. The CLI tests also cover jumps, position-based
padding, nested includes, source locations, and position assertions, including failure cases. The
x86 plugin tests cover register aliases, immediate and register-to-register
encodings, mode-aware decoding and truncated input, conditional-jump encodings and
fixups, near-jump widths, range limits, and fixups in all three modes.
Use CTest's `-V` option to show output
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
- Near conditional jumps and conditional target restoration during disassembly
- Mode-aware support for more instructions and disassembly mode selection
- KASM source assembly for register-to-register moves, additional memory forms, and broader
  disassembly support
- Completing the statement parsing contract and refining plugin language boundaries
- More instructions, registers, and addressing modes
- A complete KASM command-line workflow
- Stable releases and packaged distributions

## Releases

After publishing, the release workflow keeps the four most recently published
`kace-dev-...` development snapshots and the four most recently published
`kace-...` stable releases. Older matching releases, their assets, and their
tags are deleted. Drafts, unrelated releases, and standalone tags are not
included in this cleanup. Immutable releases cannot be removed by the
workflow; retention reports an error if an older matching release is immutable.

| Type | Description |
|------|-------------|
| Development snapshot | The current development version. It may change frequently and may contain incomplete features. |
| Stable | A tested release intended for regular use. No stable release is available yet. |

## License

KACE is distributed under the GNU General Public License, version 3 or later.
See [LICENSE](LICENSE) for the full license text.
