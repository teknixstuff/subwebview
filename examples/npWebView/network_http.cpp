#include <Windows.h>
#include "include/cef_callback.h"
#include "include/cef_request.h"
#include "include/cef_resource_handler.h"
#include "include/cef_response.h"
#include "include/cef_scheme.h"
#include <vector>
#include <string>
#include <fstream>
#include "network_http.h"
#include "npapi/npfunctions.h"
#include "npapi/npruntime.h"

struct RequestCallbackObject : public NPObject {
  NPP npp;
  CefCallback* callback;
};

static NPObject* RequestCallback_Allocate(NPP npp, NPClass* aClass) {
  RequestCallbackObject* cbObj = (RequestCallbackObject*)calloc(1, sizeof(RequestCallbackObject));
  if (cbObj) {
    cbObj->npp = npp;
  }
  return (NPObject*)cbObj;
}

static void RequestCallback_Deallocate(NPObject* npobj) {
  free(npobj);
}

static bool RequestCallback_InvokeDefault(NPObject* npobj, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
  RequestCallbackObject* cbObj = (RequestCallbackObject*)npobj;

  if (argCount > 0 && NPVARIANT_IS_STRING(args[0])) {
    NPString jsString = NPVARIANT_TO_STRING(args[0]);
  }

  VOID_TO_NPVARIANT(*result);
  return true;
}

static NPClass s_RequestCallbackClass = {
    NP_CLASS_STRUCT_VERSION,
    RequestCallback_Allocate,
    RequestCallback_Deallocate,
    NULL,
    NULL,
    NULL,
    RequestCallback_InvokeDefault,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

extern "C" __declspec(dllimport) BOOLEAN WINAPI SystemFunction036(PVOID RandomBuffer, ULONG RandomBufferLength);

std::vector<char> GetCompletePayload(CefRefPtr<CefPostData> postData) {
  std::vector<char> fullPayload;
  if (!postData) {
    return fullPayload;
  }

  CefPostData::ElementVector elements;
  postData->GetElements(elements);

  for (const auto& element : elements) {
    if (element->GetType() == PDE_TYPE_BYTES) {
      size_t size = element->GetBytesCount();
      if (size > 0) {
        size_t currentSize = fullPayload.size();
        fullPayload.resize(currentSize + size);
        element->GetBytes(size, &fullPayload[currentSize]);
      }
    } else if (element->GetType() == PDE_TYPE_FILE) {
      CefString filePath = element->GetFile();
      std::string pathStr = filePath.ToString();

      std::ifstream file(pathStr, std::ios::binary | std::ios::ate);
      if (file.is_open()) {
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        size_t currentSize = fullPayload.size();
        fullPayload.resize(currentSize + fileSize);

        file.read(fullPayload.data() + currentSize, fileSize);
      }
    }
  }

  return fullPayload;
}

std::string GenerateName() {
  BYTE bRandBytes[16];
  const char szHex[] = "0123456789ABCDEF";
  SystemFunction036(bRandBytes, sizeof(bRandBytes));
  std::string name = "SubWebView_";
  for (int i = 0; i < ARRAYSIZE(bRandBytes); i++) {
    name += szHex[bRandBytes[i] >> 4];
    name += szHex[bRandBytes[i] & 0xF];
  }
  return name;
}

NPObject* CreateJSArray(NPNetscapeFuncs pNPNFuncs, NPP npp, NPObject* windowObj) {
  NPIdentifier arrayId = pNPNFuncs.getstringidentifier("Array");
  NPVariant arrayConstructorVar;

  if (pNPNFuncs.getproperty(npp, windowObj, arrayId, &arrayConstructorVar) && NPVARIANT_IS_OBJECT(arrayConstructorVar)) {
    NPObject* arrayConstructorObj = NPVARIANT_TO_OBJECT(arrayConstructorVar);
    NPVariant resultVar;

    if (pNPNFuncs.construct(npp, arrayConstructorObj, NULL, 0, &resultVar)) {
      if (NPVARIANT_IS_OBJECT(resultVar)) {
        pNPNFuncs.releaseobject(arrayConstructorObj);
        return NPVARIANT_TO_OBJECT(resultVar);
      }
    }
    pNPNFuncs.releaseobject(arrayConstructorObj);
  }
  return NULL;
}

NPObject* ConvertHeadersToJSArray(NPNetscapeFuncs pNPNFuncs, NPP npp, const CefRequest::HeaderMap& myMap) {
  NPObject* windowObj = NULL;
  pNPNFuncs.getvalue(npp, NPNVWindowNPObject, &windowObj);

  NPObject* jsArrayObj = CreateJSArray(pNPNFuncs, npp, windowObj);
  if (!jsArrayObj) {
    pNPNFuncs.releaseobject(windowObj);
    return NULL;
  }

  uint32_t index = 0;
  NPIdentifier id0 = pNPNFuncs.getintidentifier(0);
  NPIdentifier id1 = pNPNFuncs.getintidentifier(1);

  for (auto it = myMap.begin(); it != myMap.end(); ++it, ++index) {
    NPObject* innerArrayObj = CreateJSArray(pNPNFuncs, npp, windowObj);
    if (!innerArrayObj)
      continue;

    NPVariant keyVar;
    auto keyStr = it->first.ToString();
    STRINGZ_TO_NPVARIANT(keyStr.c_str(), keyVar);
    pNPNFuncs.setproperty(npp, innerArrayObj, id0, &keyVar);

    NPVariant valVar;
    auto valStr = it->second.ToString();
    STRINGZ_TO_NPVARIANT(valStr.c_str(), valVar);
    pNPNFuncs.setproperty(npp, innerArrayObj, id1, &valVar);

    NPVariant innerArrayVar;
    OBJECT_TO_NPVARIANT(innerArrayObj, innerArrayVar);

    NPIdentifier arrayIndexId = pNPNFuncs.getintidentifier(index);
    pNPNFuncs.setproperty(npp, jsArrayObj, arrayIndexId, &innerArrayVar);

    pNPNFuncs.releaseobject(innerArrayObj);
  }

  pNPNFuncs.releaseobject(windowObj);
  return jsArrayObj;
}

typedef struct {
  HANDLE hEvent;
  LPCSTR szPostDataName;
  LPCSTR szMethod;
  LPCSTR szURI;
  UINT64 cbPostDataSize;
  CefRequest::HeaderMap* headers;
  NPP npp;
  NPNetscapeFuncs pNPNFuncs;
} RequestData;

void SendRequest(void* param) {
  RequestData* reqData = (RequestData*)param;
  auto pHeaders = ConvertHeadersToJSArray(reqData->pNPNFuncs, reqData->npp, *reqData->headers);

  NPObject* windowObj = NULL;
  reqData->pNPNFuncs.getvalue(reqData->npp, NPNVWindowNPObject, &windowObj);

  NPIdentifier makeRequestId = reqData->pNPNFuncs.getstringidentifier("subWebViewMakeRequest");
  NPVariant makeRequestVar;

  if (reqData->pNPNFuncs.getproperty(reqData->npp, windowObj, makeRequestId, &makeRequestVar) && NPVARIANT_IS_OBJECT(makeRequestVar))
  {
    NPVariant result;
    NPVariant params[4];
    STRINGZ_TO_NPVARIANT(reqData->szMethod, params[0]);
    STRINGZ_TO_NPVARIANT(reqData->szURI, params[1]);
    params[2].type = NPVariantType_Object;
    params[2].value.objectValue = pHeaders;
    STRINGZ_TO_NPVARIANT(reqData->szPostDataName, params[3]);
    reqData->pNPNFuncs.invokeDefault(reqData->npp, NPVARIANT_TO_OBJECT(makeRequestVar), params, ARRAYSIZE(params), &result);
  }
  SetEvent(reqData->hEvent);
}

class MyCustomHttpHandler : public CefResourceHandler {
 public:
  MyCustomHttpHandler(NPNetscapeFuncs pNPNFuncs, NPP npp) {
    this->pNPNFuncs = pNPNFuncs;
    this->npp = npp;
  };
  ~MyCustomHttpHandler() override = default;

  bool Open(CefRefPtr<CefRequest> request,
            bool& handle_request,
            CefRefPtr<CefCallback> callback) override
  {
    std::string url = request->GetURL();
    std::string method = request->GetMethod();

    CefRequest::HeaderMap headers;
    request->GetHeaderMap(headers);

    auto data = GetCompletePayload(request->GetPostData());
    std::string szPostDataName = "";
    HANDLE hPostData = NULL;
    if (data.size() > 0) {
      ULARGE_INTEGER ulSize;
      ulSize.QuadPart = data.size();
      szPostDataName = GenerateName();
      hPostData = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, ulSize.HighPart, ulSize.LowPart, szPostDataName.c_str());
      LPVOID pPostData = MapViewOfFile(hPostData, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      memcpy(pPostData, data.data(), data.size());
      UnmapViewOfFile(pPostData);
    }

    RequestData reqData;
    reqData.szPostDataName = szPostDataName.c_str();
    reqData.cbPostDataSize = data.size();
    reqData.headers = &headers;
    reqData.pNPNFuncs = pNPNFuncs;
    reqData.npp = npp;
    reqData.szMethod = method.c_str();
    reqData.szURI = url.c_str();

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    reqData.hEvent = hEvent;
    pNPNFuncs.pluginthreadasynccall(npp, SendRequest, &reqData);
    WaitForSingleObject(hEvent, INFINITE);
    CloseHandle(hEvent);
    CloseHandle(hPostData);

    handle_request = true;
    return true;
  }

  void GetResponseHeaders(CefRefPtr<CefResponse> response,
                          int64_t& response_length,
                          CefString& redirectUrl) override
  {
    response->SetStatus(200);
    response->SetMimeType("text/html");
    response->SetError

    response_length = -1;
  }

  bool Read(void* data_out,
            int bytes_to_read,
            int& bytes_read,
            CefRefPtr<CefResourceReadCallback> callback) override
  {
    bytes_read = 0;
    return false;
  }

  void Cancel() override {
  }

 private:
  NPNetscapeFuncs pNPNFuncs;
  NPP npp;
  IMPLEMENT_REFCOUNTING(MyCustomHttpHandler);
};

CefRefPtr<CefResourceHandler> MyHttpSchemeHandlerFactory::Create(
  CefRefPtr<CefBrowser> browser,
  CefRefPtr<CefFrame> frame,
  const CefString& scheme_name,
  CefRefPtr<CefRequest> request)
{
  return new MyCustomHttpHandler(pNPNFuncs, npp);
}

MyHttpSchemeHandlerFactory::MyHttpSchemeHandlerFactory(NPNetscapeFuncs pNPNFuncs, NPP npp) {
  this->pNPNFuncs = pNPNFuncs;
  this->npp = npp;
}