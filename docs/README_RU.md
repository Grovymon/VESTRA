# Vestra Beta

[English version](../README.md) · Русский

> [!WARNING]
> **Это бета-версия.** Vestra прошла автоматическую сборку и тесты, но пока не является заменой
> Microsoft Word и не редактирует DOCX/XLSX/PPTX без дополнительного backend. Не используйте её
> как единственную копию важного документа.

Vestra — бесплатный офисный пакет с открытым исходным кодом для Windows 10/11 x64. По умолчанию
она работает локально: без аккаунта, подписки, рекламы, обязательного облака, скрытой телеметрии
и автоматических сетевых запросов.

## Что работает в Beta

- `vestra.exe` — стартовое окно с недавними файлами, настройками, RU/EN и светлой, тёмной или
  системной темой;
- `vestra-writer.exe` — редактирование и сохранение TXT, ленточный интерфейс, форматирование,
  списки, таблицы, изображения, ссылки, поиск/замена, отмена/повтор, печать, масштаб, экспорт
  в PDF, Protected View и локальное автовосстановление;
- `vestra-sheets.exe` — локальная таблица с листами, CSV, базовым форматированием и буфером
  обмена;
- `vestra-slides.exe` — локальные презентации с текстом, фигурами, изображениями и показом;
- `vestra-pdf.exe` — просмотр PDF через Qt PDF с навигацией и масштабом;
- `vestra-document-worker.exe` — отдельный worker для предварительной проверки документов и
  безопасного IPC.

## Честные ограничения Beta

- DOCX, RTF, ODT, XLSX, XLS, ODS, PPTX, PPT и ODP требуют опциональный LibreOfficeKit backend.
  Если его нет, Vestra не имитирует поддержку и не изменяет файл молча.
- Аннотации, формы, поиск и перестановка страниц PDF пока не реализованы.
- Текущий worker ограничен Job Object Windows, но ещё не является полноценной песочницей
  AppContainer. Макросы никогда не выполняются.
- Нет установщика, автообновлений с подписью, ассоциаций файлов и полного crash-recovery UX.

## Сборка

Нужны CMake 3.24+, Ninja, Qt 6.5+ (`Core`, `Gui`, `Widgets`, `Network`, `PrintSupport`, `Pdf`,
`Svg`, `Test`) и компилятор C++20 для Windows x64.

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

## Приватность и безопасность

Данные Vestra хранятся только локально:

```text
%LOCALAPPDATA%\Vestra\
├── Config
├── Cache
├── Recovery
├── Logs
└── Temp
```

Перед открытием документ копируется во временную папку и проверяется отдельным worker-процессом.
Макросы отключены, логи не отправляются, а Protected View включён для потенциально небезопасных
файлов. Полная модель угроз описана в [THREAT_MODEL.md](THREAT_MODEL.md).

## Следующий этап

Главный этап после Beta — LibreOfficeKit внутри `office-worker`: безопасное открытие и tiled
rendering DOCX/XLSX/PPTX без прямой связи UI с движком. Затем — полноценная изоляция AppContainer
и PDF worker.

## Лицензия

Исходный код Vestra распространяется по Apache-2.0. Условия сторонних компонентов приведены в
[THIRD_PARTY_LICENSES.md](../THIRD_PARTY_LICENSES.md).

