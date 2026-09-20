# Roadmap

## 0.1 foundation (this repository)

Launcher, practical TXT Writer, optional PDFium viewer, settings/themes/RU+EN, recents, local logs,
worker IPC, structural preflight, Protected View hand-off, backend interfaces, tests and Windows CI.

## Recommended next increment

Finish the security boundary before broad format features:

1. AppContainer or restricted-token worker with explicit no-network capability and brokered handles;
2. streaming OOXML relationship/XML/image validation and processing time quotas;
3. move PDFium rendering into `pdf-worker` and transfer bounded raster/text results over IPC;
4. implement the LibreOfficeKit tiled-rendering adapter inside `office-worker`;
5. add corpus tests for clean, corrupt, macro-enabled and adversarial documents.

## 0.2

Sheets via the worker-hosted LibreOfficeKit bridge, XLSX/XLS/ODS/CSV, Writer improvements and PDF
annotations.

## 0.3

Slides, PPTX/PPT/ODP, presentation mode and an audited sandbox policy.

## 1.0

Complete app set, signed installer/updater, crash recovery UX, file associations, document corpus,
security review and end-user/admin documentation.


