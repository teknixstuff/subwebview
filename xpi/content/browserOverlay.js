Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/ctypes.jsm");
var hNtdll = ctypes.open("ntdll.dll");
const DWORD = ctypes.uint32_t;
const LPDWORD = new ctypes.PointerType(DWORD);
var build = DWORD();
hNtdll.declare("RtlGetNtVersionNumbers",ctypes.winapi_abi,DWORD,DWORD,DWORD,LPDWORD)(0,0,build.address());
hNtdll.close();
const buildNumber = build.value & 0x00FFFFFF;

if (buildNumber >= 10240) {
  // Windows 10 or later. Install the CEF 130 (W10/W11) version.
  let newAddon = {
    url: 'https://github.com/teknixstuff/subwebview/releases/download/1.0.4/subwebview@teknixstuff.com-1.0.4.xpi',
    mimetype: 'application/x-xpinstall',
    hash: undefined,
    name: 'SubWebView for Windows 10/11',
    iconURL: 'chrome://subwebview/content/logo.png',
    version: '1.0.4',
    loadGroup: undefined,
    callback: function(installObj){
      let installListener = {
        onNewInstall: function(){},
        onDownloadStarted: function(){},
        onDownloadProgress: function(){},
        onDownloadEnded: function(){},
        onDownloadCancelled: function(){},
        onDownloadFailed: function(){},
        onInstallStarted: function(){},
        onInstallEnded: function(){
              const nsIAppStartup = Components.interfaces.nsIAppStartup;

              // Notify all windows that an application quit has been requested.
              var os = Components.classes["@mozilla.org/observer-service;1"]
                  .getService(Components.interfaces.nsIObserverService);
              var cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"]
                  .createInstance(Components.interfaces.nsISupportsPRBool);
              os.notifyObservers(cancelQuit, "quit-application-requested", null);

              // Something aborted the quit process. 
              if (cancelQuit.data)
                  return;

              // Notify all windows that an application quit has been granted.
              os.notifyObservers(null, "quit-application-granted", null);

              // Enumerate all windows and call shutdown handlers
              var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                  .getService(Components.interfaces.nsIWindowMediator);
              var windows = wm.getEnumerator(null);
              while (windows.hasMoreElements()) {
                  var win = windows.getNext();
                  if (("tryToClose" in win) && !win.tryToClose())
                      return;
              }
              Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(nsIAppStartup)
                  .quit(nsIAppStartup.eRestart | nsIAppStartup.eAttemptQuit);
        },
        onInstallCancelled: function(){},
        onInstallFailed: function(){},
        onExternalInstall: function(){},
      }
      installObj.addListener(installListener);
      installObj.install();
    }
  }

  AddonManager.getInstallForURL(newAddon.url, newAddon.callback, newAddon.mimetype, newAddon.hash, newAddon.name, newAddon.iconURL, newAddon.version, newAddon.loadGroup);
}

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