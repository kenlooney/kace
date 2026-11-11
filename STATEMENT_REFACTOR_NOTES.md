# Statement refactor handoff

Work in progress as of November 10, 2026. The current working tree does not
build; these notes describe the state to resume from, not a finished release.

## Improvements before the refactor

- The x86 register file groups overlapping names and provides register widths
  and encoding IDs. The immediate encoder supports 8-, 16-, and 32-bit
  register forms with mode-dependent operand-size prefixes. Its 64-bit branch
  is unfinished: `hasWidth()` does not recognize primary names such as `rax`.
- The immediate decoder supports `B0`–`B7` byte-register moves and `B8`–`BF`
  for the first eight 16-, 32-, and 64-bit register groups, including the `66`
  operand-size prefix and `48` REX.W form. Other REX prefixes and extended
  registers are not covered.
- Focused x86 decoder tests passed across supported widths, modes, register
  names, nonzero offsets, and incomplete input before the current refactor.
- A `mov` branch was added to the x86 assembler to evaluate the immediate,
  select the current mode, call the encoder, and append its output. This CLI
  path has not yet been verified end to end.
- The assembler contract and KASM statement pipeline were changed to carry
  `std::unique_ptr<Statement>` so derived statements are not sliced. A test
  previously verified that a `KasmStatement` and its move kind survived the
  contract, before the subsequent statement redesign.

## Current design direction

Keep `Statement` as a reusable framework base. It currently holds a statement
name and a collection of named `StatementKind` values, with getters, setters,
`add_kind()`, and `has_kind()` overloads. Its `parse_statement()` is pure
virtual. `KasmStatement` derives from it and currently contains `sourceName`,
`line`, `type`, `value`, and `moveKind`; more KASM-specific data still needs a
home. `MoveKind` includes immediate/register/memory combinations and `None`;
only immediate moves were previously parsed and encoded.

`StatementKind` objects are currently stored by value: derived kind objects
would be sliced. Named kinds work as labels, but using a kind to carry extra
data would require a different storage and ownership design.

## Where the build stopped

- `KasmStatement` is abstract because it does not yet implement
  `parse_statement()`. `KasmParser` still constructs it directly.
- `KasmParser`, include expansion, the x86 assembler, and tests still use old
  statement fields and enum values, such as `statement.kind`,
  `StatementKind::Directive`, `statement.name`, expression data, jump targets,
  directive fields, and code-segment fields. The old field list is commented
  out in `targets/kace/include/kace/statement.hpp` for reference.
- Byte validation was moved from KACE to `kasmcore` as `check_kasm_byte()`.
  It checks for the named `Times` kind; the parser has begun adding that kind.
  The x86 plugin now links to `kasmcore` and expects `KasmStatement` objects.
  Other statement accesses in that plugin remain to be migrated.
- The loader test now compiles with a test-only derived statement, but it has
  not been run against the refactored plugin because the full project does not
  build. The test-only parse override intentionally reports that parsing is
  unused; it is not the real KASM implementation.
- The previous build reached `kasmparser.hpp` and reported the abstract class,
  missing kind values, and missing fields. Many repeated errors result from
  the same unfinished migration. Further errors may appear as files compile.

## Suggested next steps

1. Decide how `KasmStatement::parse_statement()` relates to the existing
   `KasmParser`, which already parses source statements. Avoid a placeholder
   that silently reports successful parsing.
2. Move the required fields into `KasmStatement` and consistently replace old
   enum checks with the chosen named-kind representation. Preserve source
   locations, directives, expressions, and jump behavior.
3. Update include expansion and the x86 assembler to access the derived
   statement safely. Account for existing tests that construct base statements
   and for the plugin's new KASM dependency.
4. Build incrementally, run the complete CTest suite, and add end-to-end tests
   for immediate moves and the new statement representation. Update README and
   website claims after behavior has been verified.

Keep the current uncommitted changes, including the website bio edits, intact.
