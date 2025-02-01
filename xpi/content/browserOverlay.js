Components.utils.import("resource://gre/modules/Services.jsm");
function getEngineForURI(uri) {
  try {
    sitesConfig = JSON.parse(Services.prefs.getCharPref('extensions.subwebview.siteConfig'));
  } catch {
    sitesConfig = [];
  }
  for (const site of sitesConfig) {
    if (site.pattern == '') {
      // Default engine
      if (uri.startsWith('http://') || uri.startsWith('https://')) {
        return site.engine;
      }
    } else if (site.pattern.startsWith('/')) {
      // RegEx pattern
      try {
        if (new RegExp(site.pattern).test(uri)) {
          return site.engine;
        }
      } catch {
      }
    } else {
      // Glob pattern
      let regex = site.pattern
      .replace(/[.+?^${}()|[\]\\]/g, "\\$&")
      .replace(/^([^:]*:\/*[^\/]+\/[^]*)\*/, '$1[^]*')
      .replace(/^([^:]*:\/*[^\/]*)\*/, '$1[^\/]*')
      .replace(/^([^:]*)\*/, '$1[^:]*');
      if (new RegExp(regex).test(uri)) {
        return site.engine;
      }
    }
  }
  return 'standard';
}

let progListener = {
  init: function() {
    gBrowser.browsers.forEach(function (browser) {
      this._toggleProgressListener(browser.webProgress, true);
    }, this);

    gBrowser.tabContainer.addEventListener("TabOpen", this, false);
    gBrowser.tabContainer.addEventListener("TabClose", this, false);
  },

  uninit: function() {
    gBrowser.browsers.forEach(function (browser) {
      this ._toggleProgressListener(browser.webProgress, false);
    }, this);

    gBrowser.tabContainer.removeEventListener("TabOpen", this, false);
    gBrowser.tabContainer.removeEventListener("TabClose", this, false);
  },

  handleEvent: function(aEvent) {
    let tab = aEvent.target;
    let webProgress = gBrowser.getBrowserForTab(tab).webProgress;

    this._toggleProgressListener(webProgress, ("TabOpen" == aEvent.type));
  },

  QueryInterface: function(aIID){
      if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
         aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
         aIID.equals(Components.interfaces.nsISupports))
            return this;
      throw Components.results.NS_NOINTERFACE;
  },

  onLocationChange: function(aWebProgress, aRequest, aLocation) {
    if (getEngineForURI(aLocation.spec) != 'standard') {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
    }
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP && aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      let engine = getEngineForURI(aWebProgress.DOMWindow.location.href);
      if (engine != 'standard') {
        let doc = aWebProgress.DOMWindow.document;
        doc.documentElement.style.width = '100%';
        doc.documentElement.style.height = '100%';
        doc.documentElement.style.overflow = 'hidden';
        let metaViewport = doc.createElement('meta');
        metaViewport.name = 'viewport';
        metaViewport.content = 'width=device-width; height=device-height;';
        doc.head.appendChild(metaViewport);
        let linkIcon = doc.createElement('link');
        linkIcon.rel = 'icon';
        linkIcon.href = `http://www.google.com/s2/favicons?sz=64&domain=${aWebProgress.DOMWindow.location.host}`;
        doc.head.appendChild(linkIcon);
        doc.body.setAttribute('marginwidth', '0');
        doc.body.setAttribute('marginheight', '0');
        doc.body.style.width = '100%';
        doc.body.style.height = '100%';
        doc.body.style.overflow = 'hidden';
        let embed = doc.createElement('embed');
        embed.name = 'plugin';
        if (engine == 'chromium') {
          embed.type = 'application/x-subwebview';
        }
        if (engine == 'iexplore') {
          embed.type = 'application/x-subwebview-ie';
        }
        embed.width = '100%';
        embed.height = '100%';
        embed.src = aWebProgress.DOMWindow.location.href;
        doc.body.appendChild(embed);
      }
    }
  },

  onProgressChange: function() {},
  onStatusChange: function() {},
  onSecurityChange: function() {},
  onLinkIconAvailable: function() {},

  _toggleProgressListener: function(aWebProgress, aIsAdd) {
    if (aIsAdd) {
      aWebProgress.addProgressListener(this, aWebProgress.NOTIFY_ALL);
    } else {
      aWebProgress.removeProgressListener(this);
    }
  }
};
window.addEventListener('load', ()=>progListener.init());