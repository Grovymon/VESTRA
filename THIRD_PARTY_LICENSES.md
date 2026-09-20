# Third-party components

No third-party source code or prebuilt engine binaries are vendored in this repository.

| Component | Use | License | Included by default |
|---|---|---|---|
| Qt 6 | Core, GUI, Widgets, Network, PrintSupport, PDF, Test | LGPL-3.0 / GPL / commercial, by module and distribution | Build dependency, not vendored |
| LibreOfficeKit | Optional office document backend | MPL-2.0; contributions are commonly dual MPL-2.0/LGPL-3.0-or-later; bundled component licenses also apply | No |
| PDFium | PDF engine used by Qt PDF; optional direct adapter also exists | BSD-3-Clause plus licenses of bundled third-party components | Through the external Qt PDF binary, not vendored |

Distributors are responsible for complying with the exact Qt, LibreOffice and PDFium packages they
ship, retaining notices, offering relinkable LGPL components where required, and reproducing
PDFium's generated `LICENSES`/notice bundle. Enabling a CMake flag does not download or redistribute
the dependency.

Build tooling referenced by CI:

| Tool/action | License |
|---|---|
| `actions/checkout` | MIT |
| `jurplel/install-qt-action` | MIT |
| `ilammy/msvc-dev-cmd` | MIT |

