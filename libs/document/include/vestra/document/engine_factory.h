#pragma once

#include <memory>

namespace vestra::document {

class DocumentEngine;
class PdfEngine;

[[nodiscard]] std::unique_ptr<DocumentEngine> createDocumentEngine();
[[nodiscard]] std::unique_ptr<PdfEngine> createPdfEngine();

} // namespace vestra::document

