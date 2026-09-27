Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/ctypes.jsm");

function createInputStreamFromSection(sectionName, size) {
    var kernel32 = ctypes.open("kernel32.dll");

    const HANDLE = ctypes.intptr_t;
    const LPVOID = ctypes.void_t.ptr;
    const DWORD = ctypes.uint32_t;
    const BOOL = ctypes.int;
    const WCHAR = ctypes.char16_t;
    
    const FILE_MAP_READ = 0x0004;

    var OpenFileMappingW = kernel32.declare("OpenFileMappingW", ctypes.winapi_abi, HANDLE, DWORD, BOOL, WCHAR.ptr);

    var MapViewOfFile = kernel32.declare("MapViewOfFile", ctypes.winapi_abi, LPVOID, HANDLE, DWORD, DWORD, DWORD, DWORD);
    var UnmapViewOfFile = kernel32.declare("UnmapViewOfFile", ctypes.winapi_abi, BOOL, LPVOID);
    var CloseHandle = kernel32.declare("CloseHandle", ctypes.winapi_abi, BOOL, HANDLE);

    let hMapFile = OpenFileMappingW(FILE_MAP_READ, false, sectionName);
    if (hMapFile == HANDLE(0)) {
        throw new Error("Failed to open file mapping. Error code: " + ctypes.winLastError);
    }

    let pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, size);
    if (pBuf == LPVOID(0)) {
        CloseHandle(hMapFile);
        throw new Error("Failed to map view of file. Error code: " + ctypes.winLastError);
    }

    var arrayType = ctypes.uint8_t.array(size);
    var castedPtr = ctypes.cast(pBuf, arrayType.ptr);

    var byteArray = new Uint8Array(castedPtr);
    var stream = Cc['@mozilla.org/io/arraybuffer-input-stream;1'].createInstance(Ci.nsIArrayBufferInputStream);
    stream.setData(byteArray.buffer, 0, byteArray.buffer.byteLength);

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return stream;
}

class Uint8ArrayStreamListener {
  constructor() {
    this._chunks = [];
    this._totalLength = 0;
    this.complete = new Promise((resolve, reject)=>{
      this._resolve = resolve;
      this._reject = reject;
    });
  }

  QueryInterface(aIID) {
    if (
      aIID.equals(Ci.nsIStreamListener) ||
      aIID.equals(Ci.nsIRequestObserver) ||
      aIID.equals(Ci.nsISupports)
    ) {
      return this;
    }
    throw Components.results.NS_NOINTERFACE;
  }

  onStartRequest(aRequest, aContext) {
    this._chunks = [];
    this._totalLength = 0;
  }

  onDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount) {
    let binaryStream = Cc["@mozilla.org/binaryinputstream;1"]
                         .createInstance(Ci.nsIBinaryInputStream);
    binaryStream.setInputStream(aInputStream);

    let byteArray = binaryStream.readByteArray(aCount);
    let uint8Chunk = new Uint8Array(byteArray);

    this._chunks.push(uint8Chunk);
    this._totalLength += uint8Chunk.length;
  }

  onStopRequest(aRequest, aContext, aStatusCode) {
    let finalBuffer = new Uint8Array(this._totalLength);
    let offset = 0;
    
    for (let chunk of this._chunks) {
      finalBuffer.set(chunk, offset);
      offset += chunk.length;
    }

    this.buffer = finalBuffer;
    this._resolve(aStatusCode);
  }
}

function makeRequest(method, uri, headers, body, callback, document) {
    let channel = Services.io.newChannel2(
        uri, 
        null, null, document, 
        Services.scriptSecurityManager.getSystemPrincipal(),
        null,
        Ci.nsILoadInfo.SEC_ALLOW_CROSS_ORIGIN_DATA_IS_NULL,
        Ci.nsIContentPolicy.TYPE_OTHER
    );

    if (body) {
        let uploadChannel = channel.QueryInterface(Ci.nsIUploadChannel);
        let inputStream = createInputStreamFromSection(body.sectionName, body.size);
        uploadChannel.setUploadStream(inputStream, "", body.size);
    }

    let httpChannel = channel.QueryInterface(Ci.nsIHttpChannel);
    httpChannel.requestMethod = method;
    headers.forEach(header=>{
        httpChannel.setRequestHeader(header[0], header[1], true);
    });

    let stream = new Uint8ArrayStreamListener();
    channel.asyncOpen(stream, null);
    stream.complete.then(status=>{
        if (Components.isSuccessCode(status)) {
        } else {
        }
    });

    let request = channel.QueryInterface(Ci.nsIRequest);
    return function(){
        request.cancel(Cr.NS_ERROR_ABORT);
    };
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
        let profile = Components.classes["@mozilla.org/file/directory_service;1"]
                     .getService(Components.interfaces.nsIProperties)
                     .get("ProfD", Components.interfaces.nsILocalFile);
        aWebProgress.DOMWindow.wrappedJSObject.subWebViewProfile = profile.path;
        aWebProgress.DOMWindow.wrappedJSObject.subWebViewMakeRequest = Cu.exportFunction(function(method, uri, headers, callback, body) {
          return makeRequest(method, uri, headers, body, callback, doc);
        }, aWebProgress.DOMWindow);
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

let appShellService = Components.classes["@mozilla.org/appshell/appShellService;1"]
                                .getService(Components.interfaces.nsIAppShellService);
let hiddenWin = appShellService.hiddenDOMWindow;

if (typeof hiddenWin.subWebView_cspObserver === 'undefined') {
  hiddenWin.subWebView_cspObserver = {
    isRegistered: false,

    init: function() {
      if (this.isRegistered) return;
      let obsService = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
      obsService.addObserver(this, "http-on-examine-response", false);
      this.isRegistered = true;
    },

    uninit: function() {
      if (!this.isRegistered) return;
      let obsService = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
      obsService.removeObserver(this, "http-on-examine-response");
      this.isRegistered = false;
    },

    observe: function(subject, topic, data) {
      if (topic === "http-on-examine-response") {
        let channel = subject.QueryInterface(Ci.nsIHttpChannel);
      
        let engine = getEngineForURI(channel.URI.spec);
        if (engine != 'standard') {
          try {
            channel.setResponseHeader("Content-Security-Policy", "", false);
          } catch (e) {}
          try {
            channel.setResponseHeader("Content-Security-Policy-Report-Only", "", false);
          } catch (e) {}
          try {
            channel.setResponseHeader("X-Content-Security-Policy", "", false);
          } catch (e) {}
        }
      }
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupports])
  };
}
window.addEventListener('load', ()=>hiddenWin.subWebView_cspObserver.init());