# Vestra Beta

[Русская версия](docs/README_RU.md) · English

> [!WARNING]
> **Beta software.** Vestra has passed automated build and test checks, but it is not yet a
> replacement for Microsoft Word or a general DOCX/XLSX/PPTX editor. Do not use it as the only
> copy of an important document.

Vestra is an open-source, offline-first office shell for Windows 10/11 x64. It has no
accounts, subscriptions, advertising, mandatory cloud services, telemetry uploads, or
automatic network activity.

This repository contains a **compilable office-suite foundation**, not a claim of complete
Microsoft Office compatibility. It deliberately uses a familiar ribbon workflow without copying
Microsoft assets or pixel-identical layouts. File-format compatibility belongs to replaceable
engine adapters:

```text
Qt UI -> Vestra Core API -> Document/PDF adapters -> LibreOfficeKit / PDFium
                    \\-> bounded local IPC -> document worker
```

The UI never links directly to LibreOfficeKit or PDFium. Qt PDF is enabled by default as the
packaged PDFium-based renderer; the direct PDFium and LibreOfficeKit adapters remain replaceable.

## Current status

Implemented:

- `vestra.exe`: Office-style launcher for all four applications, file picker, recent files,
  settings, RU/EN and light/dark/system themes;
- `vestra-writer.exe`: local TXT editing and saving, page-oriented editing workspace, rich-text
  formatting, lists, tables, images, links, find/replace, undo/redo, print, zoom, PDF export,
  Protected View and local recovery snapshots;
- `vestra-sheets.exe`: editable multi-sheet grid, formula bar, CSV open/save, clipboard,
  undo/redo, row/column insertion, formatting and sorting;
- `vestra-slides.exe`: editable slide canvas, thumbnails, add/delete/duplicate/reorder,
  text, images, shapes, background colors, full-screen presentation, local save and PDF export;
- `vestra-pdf.exe`: bundled PDFium-based page rendering, navigation and zoom through Qt PDF;
- `vestra-document-worker.exe`: one-shot authenticated local IPC service and document preflight;
- file type routing, PDF signature checks, macro-capable format detection and Protected View hand-off;
- local settings and date-scoped local log files under `%LOCALAPPDATA%\Vestra`;
- CTest coverage for Core, IPC framing/validation, settings/localization, security limits, a
  real worker round trip and real generated-PDF rendering.

Explicitly not implemented in 0.1:

- LibreOfficeKit tiled rendering/editing bridge (the adapter boundary and dependency checks exist);
- direct tiled editing of DOCX/XLSX/PPTX through LibreOfficeKit;
- native formula evaluation and native OOXML/ODF persistence in Sheets/Slides;
- PDF annotations/forms/search and PDF page manipulation;
- AppContainer network/file ACL isolation, installer, updater, signatures and file associations.

The Launcher starts Writer, Sheets, Slides and PDF as separate processes. TXT, CSV, native Vestra
presentations and PDF rendering work without the office backend. DOCX/XLSX/PPTX, RTF and ODT/ODS/
ODP are routed to the correct application, which states that LibreOfficeKit is required; it does
not fabricate document content or silently change the format.

## Build on Windows

Requirements:

- Windows 10 or 11 x64;
- Visual Studio 2022 Build Tools with the Desktop C++ workload;
- CMake 3.24+ and Ninja (or the Visual Studio generator);
- Qt 6.5+ with Core, Gui, Widgets, Network, PrintSupport, PDF and Test.

From an **x64 Native Tools Command Prompt for VS 2022**:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64 `
  -DBUILD_TESTING=ON `
  -DVESTRA_ENABLE_LIBREOFFICE=OFF `
  -DVESTRA_ENABLE_PDFIUM=OFF `
  -DVESTRA_ENABLE_QT_PDF=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
build\bin\vestra.exe
```

For a distributable directory, run `windeployqt` on all five GUI executables and include
`vestra-document-worker.exe` beside them.

### Optional backends

LibreOfficeKit configuration validates that its headers and library are present, but the
0.1 adapter intentionally reports that tiled integration is unfinished:

```powershell
cmake -S . -B build-lok -DVESTRA_ENABLE_LIBREOFFICE=ON `
  -DVESTRA_LIBREOFFICE_ROOT=C:\path\to\libreoffice
```

Qt PDF provides the default PDFium-backed renderer without requiring a separately assembled
PDFium SDK. It can be disabled with `-DVESTRA_ENABLE_QT_PDF=OFF`.

The direct PDFium adapter can be selected instead:

```powershell
cmake -S . -B build-pdfium -DVESTRA_ENABLE_QT_PDF=OFF `
  -DVESTRA_ENABLE_PDFIUM=ON `
  -DVESTRA_PDFIUM_ROOT=C:\path\to\pdfium
```

The PDFium distribution must provide `include/fpdfview.h` and `lib/pdfium.lib` (plus its
runtime DLL beside `vestra-pdf.exe`, if dynamically linked).

## Local data and privacy

```text
%LOCALAPPDATA%\Vestra\
├── Config
├── Cache
├── Recovery
├── Logs
└── Temp
```

Vestra Beta contains no telemetry client, account system, cloud integration, update request,
advertising feed or other networking code. Enabling the future update-consent checkbox stores
only local consent; it does not perform a request in this milestone.

## Security boundary

Documents selected by the Launcher, Writer or PDF app are copied into an unpredictable,
per-request directory and inspected by a separate worker. The protocol uses random server names,
a 256-bit session token, same-user local-server access, a 1 MiB message limit and strict JSON
field validation. The worker accepts only a read-only `input.*` file below the supplied root.

On Windows the launcher assigns the worker to a Job Object with kill-on-close, one-process and
512 MiB memory limits and starts it with a reduced environment. This is useful process containment,
but **is not an AppContainer** and does not yet enforce a network deny ACL. See
[`SECURITY.md`](SECURITY.md) and [`docs/THREAT_MODEL.md`](docs/THREAT_MODEL.md).

## License

Vestra source is licensed under Apache-2.0. Optional dependencies keep their own licenses; see
[`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md). No Microsoft Office source code, assets,
trademarks or pixel-identical design are included.

