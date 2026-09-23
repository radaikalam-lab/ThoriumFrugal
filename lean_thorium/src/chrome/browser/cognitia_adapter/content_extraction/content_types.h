#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_CONTENT_EXTRACTION_CONTENT_TYPES_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_CONTENT_EXTRACTION_CONTENT_TYPES_H_

#include <cstddef>
#include <string>

namespace cognitia {

enum class ContentExtractionMode {
  kSelectedText,      // User-selected highlighted text
  kMainPageContent,   // Visible, sanitized structured text of the main document
  kMetadataOnly       // Title, URL, origin, and high-level headings only
};

inline const char* ContentExtractionModeToString(ContentExtractionMode mode) {
  switch (mode) {
    case ContentExtractionMode::kSelectedText:
      return "selected_text";
    case ContentExtractionMode::kMainPageContent:
      return "main_page_content";
    case ContentExtractionMode::kMetadataOnly:
      return "metadata_only";
  }
  return "unknown_mode";
}

// Strict content bounds to prevent memory bloat and IPC saturation
constexpr size_t kMaxSelectedTextBytes = 64 * 1024;    // 64 KB
constexpr size_t kMaxPageContentBytes = 256 * 1024;    // 256 KB
constexpr size_t kMaxMetadataBytes = 16 * 1024;        // 16 KB
constexpr size_t kMaxDomNodesToTraverse = 5000;        // Max DOM elements visited

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_CONTENT_EXTRACTION_CONTENT_TYPES_H_
