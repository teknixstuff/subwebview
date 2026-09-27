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

std::wstring GenerateName() {
  BYTE bRandBytes[16];
  const WCHAR szHex[] = L"0123456789ABCDEF";
  SystemFunction036(bRandBytes, sizeof(bRandBytes));
  std::wstring name = L"SubWebView_";
  for (int i = 0; i < ARRAYSIZE(bRandBytes); i++) {
    name += szHex[bRandBytes[i] >> 4];
    name += szHex[bRandBytes[i] & 0xF];
  }
  return name;
}

void SendRequest(void* param) {

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
    std::wstring szPostDataName = L"";
    HANDLE hPostData = NULL;
    if (data.size() > 0) {
      ULARGE_INTEGER ulSize;
      ulSize.QuadPart = data.size();
      szPostDataName = GenerateName();
      hPostData = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, ulSize.HighPart, ulSize.LowPart, szPostDataName.c_str());
      LPVOID pPostData = MapViewOfFile(hPostData, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      memcpy(pPostData, data.data(), data.size());
      UnmapViewOfFile(pPostData);
    }

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    pNPNFuncs.pluginthreadasynccall(npp, SendRequest, nullptr);
    WaitForSingleObject(hEvent, INFINITE);
    CloseHandle(hEvent);

    handle_request = false;
    return true;
  }

  void GetResponseHeaders(CefRefPtr<CefResponse> response,
                          int64_t& response_length,
                          CefString& redirectUrl) override
  {
    response->SetStatus(200);
    response->SetMimeType("text/html");

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