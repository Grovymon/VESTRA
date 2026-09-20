# Contributing to Vestra

Vestra accepts focused changes that preserve offline-first behavior, replaceable backends and the
worker security boundary.

1. Build with the optional engines disabled.
2. Run `clang-format` on changed C++ files.
3. Run `ctest --test-dir build --output-on-failure`.
4. Add or update English and Russian catalog entries together.
5. Document every new dependency and its license in `THIRD_PARTY_LICENSES.md` before use.
6. Never add telemetry, account, cloud or update traffic without explicit project approval and a
   visible opt-in design.

Avoid hard-coded user-facing text, unsafe deserialization, unbounded messages, `system()`, shell
execution, engine types in UI headers and document parsing in the launcher process.


