#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_OBSERVATION_BROWSER_OBSERVATION_COLLECTOR_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_OBSERVATION_BROWSER_OBSERVATION_COLLECTOR_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "chrome/browser/cognitia_adapter/observation/observation_types.h"
#include "chrome/browser/cognitia_adapter/transport/cognitia_transport.h"

namespace content {
class WebContents;
class NavigationHandle;
}  // namespace content

namespace cognitia {

// Top-level observation bridge monitoring TabStrip and Navigation events
class BrowserObservationCollector : public TabStripModelObserver,
                                    public content::WebContentsObserver {
 public:
  explicit BrowserObservationCollector(std::unique_ptr<CognitiaTransport> transport);
  ~BrowserObservationCollector() override;

  void SetUrlExposurePolicy(UrlExposurePolicy policy) { url_policy_ = policy; }
  void SetEnabled(bool enabled) { is_enabled_ = enabled; }
  bool is_enabled() const { return is_enabled_; }

  // TabStripModelObserver implementations
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // content::WebContentsObserver implementations
  void DidStartNavigation(content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(content::NavigationHandle* navigation_handle) override;
  void TitleWasSet(content::NavigationEntry* entry) override;

  // Emits a custom observation envelope asynchronously
  void EmitEvent(BrowserEventType event_type,
                 int window_id,
                 int tab_id,
                 bool is_active_tab,
                 const GURL& url,
                 const std::u16string& title,
                 bool is_incognito);

 private:
  bool is_enabled_ = false;
  UrlExposurePolicy url_policy_ = UrlExposurePolicy::kFullUrlSanitized;
  std::unique_ptr<CognitiaTransport> transport_;

  base::WeakPtrFactory<BrowserObservationCollector> weak_factory_{this};
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_OBSERVATION_BROWSER_OBSERVATION_COLLECTOR_H_
