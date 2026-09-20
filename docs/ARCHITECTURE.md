# Architecture

## Dependency direction

```text
apps/* -> libs/ui -> libs/settings -> libs/core
   |          |                         ^
   |          +-------------------------+
   +-> libs/document (abstract engines + adapters)
   +-> libs/sandbox -> libs/ipc

workers/* -> libs/ipc + libs/security -> libs/core
```

`DocumentEngine` and `PdfEngine` are the only document-backend contracts exposed to applications.
Qt widgets must not include LibreOfficeKit or PDFium headers. Optional SDK headers and libraries are
private to `vestra_document`.

Every application is a separate executable. Opening Writer does not initialize PDFium; opening the
PDF app does not initialize LibreOfficeKit; Sheets and Slides are not built in milestone 0.1.

## Process flow

1. The user selects a file.
2. The app copies it to `%LOCALAPPDATA%\Vestra\Temp\job-<random>\input.<ext>`.
3. The app starts `vestra-document-worker` with a random pipe name and session token.
4. The worker validates the request/path and performs bounded structural preflight.
5. The worker returns only result codes and findings, then exits.
6. The app removes the temporary directory and routes the original path to the chosen UI.

Engine parsing/rendering must move behind worker APIs as the adapters mature. PDFium in-process
rendering is opt-in scaffolding in 0.1 and must not be treated as the final security boundary.


