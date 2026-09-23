#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_CONTENT_EXTRACTION_CONTENT_EXTRACTOR_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_CONTENT_EXTRACTION_CONTENT_EXTRACTOR_H_

#include <string>
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/cognitia_adapter/content_extraction/content_types.h"
#include "chrome/browser/cognitia_adapter/protocol/content_envelope.h"

namespace content {
class WebContents;
}  // namespace content

namespace cognitia {

using ContentExtractionCallback = base::OnceCallback<void(std::unique_ptr<ContentEnvelope>)>;

class ContentExtractor {
 public:
  ContentExtractor();
  ~ContentExtractor();

  // Asynchronously extracts user-authorized content from WebContents
  // Ensures zero blocking of UI/renderer threads; enforces structural security exclusions.
  static void ExtractAuthorizedContent(
      content::WebContents* web_contents,
      ContentExtractionMode mode,
      const std::string& selection_text,
      ContentExtractionCallback callback);

  // Pure text sanitization function (structural exclusion of scripts, inputs, credentials)
  static std::string SanitizeRawText(const std::string& raw_input, size_t max_bytes, bool* out_truncated);
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_CONTENT_EXTRACTION_CONTENT_EXTRACTOR_H_
