#pragma once
#include "include/cef_scheme.h"
#include "npapi/npfunctions.h"

class MyHttpSchemeHandlerFactory : public CefSchemeHandlerFactory {
 public:
  MyHttpSchemeHandlerFactory(NPNetscapeFuncs pNPNFuncs, NPP npp);
  CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame,
                                       const CefString& scheme_name,
                                       CefRefPtr<CefRequest> request) override;

  IMPLEMENT_REFCOUNTING(MyHttpSchemeHandlerFactory);

 private:
  NPNetscapeFuncs pNPNFuncs;
  NPP npp;
};