#include "chrome/browser/cognitia_adapter/content_extraction/content_extractor.h"

#include <algorithm>
#include "base/task/thread_pool.h"
#include "content/public/browser/web_contents.h"
#include "chrome/browser/cognitia_adapter/protocol/url_sanitizer.h"

namespace cognitia {

ContentExtractor::ContentExtractor() = default;
ContentExtractor::~ContentExtractor() = default;

std::string ContentExtractor::SanitizeRawText(const std::string& raw_input,
                                              size_t max_bytes,
                                              bool* out_truncated) {
  *out_truncated = false;
  if (raw_input.empty()) {
    return "";
  }

  std::string sanitized = raw_input;

  // Enforce explicit size budget
  if (sanitized.size() > max_bytes) {
    sanitized.resize(max_bytes);
    *out_truncated = true;
  }

  return sanitized;
}

void ContentExtractor::ExtractAuthorizedContent(
    content::WebContents* web_contents,
    ContentExtractionMode mode,
    const std::string& selection_text,
    ContentExtractionCallback callback) {
  if (!web_contents) {
    std::move(callback).Run(nullptr);
    return;
  }

  GURL raw_url = web_contents->GetVisibleURL();
  std::string sanitized_url = UrlSanitizer::SanitizeUrl(raw_url, UrlExposurePolicy::kFullUrlSanitized);
  std::string origin = raw_url.DeprecatedGetOriginAsURL().spec();
  std::u16string title = web_contents->GetTitle();

  std::string extracted_text;
  bool is_truncated = false;

  if (mode == ContentExtractionMode::kSelectedText) {
    extracted_text = SanitizeRawText(selection_text, kMaxSelectedTextBytes, &is_truncated);
  } else if (mode == ContentExtractionMode::kMetadataOnly) {
    extracted_text = "Title: " + base::UTF16ToUTF8(title) + "\nOrigin: " + origin;
    extracted_text = SanitizeRawText(extracted_text, kMaxMetadataBytes, &is_truncated);
  } else {
    // Mode B: Main page content
    extracted_text = SanitizeRawText(selection_text, kMaxPageContentBytes, &is_truncated);
  }

  auto envelope = std::make_unique<ContentEnvelope>(
      mode, 1, 0, sanitized_url, origin, title, extracted_text, is_truncated);

  std::move(callback).Run(std::move(envelope));
}

}  // namespace cognitia
