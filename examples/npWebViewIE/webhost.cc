#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <exdisp.h>
#include "webhost.h"

#define NOTIMPLEMENTED __debugbreak(); return E_NOTIMPL

basewnd::basewnd() : mcRef(1) {}

basewnd::~basewnd() {}

ULONG basewnd::AddRef() {
  return mcRef++;
}

ULONG basewnd::Release() {
  if (--mcRef)
    return mcRef;
  delete this;
  return 0;
}


// IUnknown 

STDMETHODIMP CNullStorage::QueryInterface(REFIID riid,void ** ppvObject)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP_(ULONG) CNullStorage::AddRef(void)
{
  return 1;
}

STDMETHODIMP_(ULONG) CNullStorage::Release(void)
{
  return 1;
}


// IStorage
STDMETHODIMP CNullStorage::CreateStream(const WCHAR * pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2,IStream ** ppstm)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::OpenStream(const WCHAR * pwcsName,void * reserved1,DWORD grfMode,DWORD reserved2,IStream ** ppstm)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::CreateStorage(const WCHAR * pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2,IStorage ** ppstg)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::OpenStorage(const WCHAR * pwcsName,IStorage * pstgPriority,DWORD grfMode,SNB snbExclude,DWORD reserved,IStorage ** ppstg)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::CopyTo(DWORD ciidExclude,IID const * rgiidExclude,SNB snbExclude,IStorage * pstgDest)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::MoveElementTo(const OLECHAR * pwcsName,IStorage * pstgDest,const OLECHAR* pwcsNewName,DWORD grfFlags)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::Commit(DWORD grfCommitFlags)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::Revert(void)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::EnumElements(DWORD reserved1,void * reserved2,DWORD reserved3,IEnumSTATSTG ** ppenum)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::DestroyElement(const OLECHAR * pwcsName)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::RenameElement(const WCHAR * pwcsOldName,const WCHAR * pwcsNewName)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::SetElementTimes(const WCHAR * pwcsName,FILETIME const * pctime,FILETIME const * patime,FILETIME const * pmtime)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::SetClass(REFCLSID clsid)
{
  return S_OK;
}

STDMETHODIMP CNullStorage::SetStateBits(DWORD grfStateBits,DWORD grfMask)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CNullStorage::Stat(STATSTG * pstatstg,DWORD grfStatFlag)
{
  NOTIMPLEMENTED;
}




STDMETHODIMP CMySite::QueryInterface(REFIID riid,void ** ppvObject)
{
  if(riid == IID_IUnknown || riid == IID_IOleClientSite)
    *ppvObject = (IOleClientSite*)this;
  else if(riid == IID_IOleInPlaceSite) // || riid == IID_IOleInPlaceSiteEx || riid == IID_IOleInPlaceSiteWindowless)
    *ppvObject = (IOleInPlaceSite*)this;
  else
  {
    *ppvObject = NULL;
    return E_NOINTERFACE;
  }

  return S_OK;
}

STDMETHODIMP_(ULONG) CMySite::AddRef(void)
{
  return 1;
}

STDMETHODIMP_(ULONG) CMySite::Release(void)
{
  return 1;
}


// IOleClientSite

STDMETHODIMP CMySite::SaveObject()
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMySite::GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker ** ppmk)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMySite::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
  // We are a simple object and don't support a container.
  *ppContainer = NULL;

  return E_NOINTERFACE;
}

STDMETHODIMP CMySite::ShowObject()
{
//  NOTIMPLEMENTED;
  // huh?
  return NOERROR;
}

STDMETHODIMP CMySite::OnShowWindow(BOOL fShow)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMySite::RequestNewObjectLayout()
{
  NOTIMPLEMENTED;
}

// IOleWindow

STDMETHODIMP CMySite::GetWindow(HWND FAR* lphwnd)
{
  *lphwnd = host->hwnd;

  return S_OK;
}

STDMETHODIMP CMySite::ContextSensitiveHelp(BOOL fEnterMode)
{
  NOTIMPLEMENTED;
}

// IOleInPlaceSite


STDMETHODIMP CMySite::CanInPlaceActivate()
{
  // Yes we can
  return S_OK;
}

STDMETHODIMP CMySite::OnInPlaceActivate()
{
  // Why disagree.
  return S_OK;
}

STDMETHODIMP CMySite::OnUIActivate()
{
  return S_OK;
}

STDMETHODIMP CMySite::GetWindowContext(
  LPOLEINPLACEFRAME FAR* ppFrame,
  LPOLEINPLACEUIWINDOW FAR* ppDoc,
  LPRECT prcPosRect,
  LPRECT prcClipRect,
  LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
  *ppFrame = &host->frame;
  *ppDoc = NULL;
  GetClientRect(host->hwnd,prcPosRect);
  GetClientRect(host->hwnd,prcClipRect);

  lpFrameInfo->fMDIApp = FALSE;
  lpFrameInfo->hwndFrame = host->hwnd;
  lpFrameInfo->haccel = NULL;
  lpFrameInfo->cAccelEntries = 0;

  return S_OK;
}

STDMETHODIMP CMySite::Scroll(SIZE scrollExtent)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMySite::OnUIDeactivate(BOOL fUndoable)
{
  return S_OK;
}

STDMETHODIMP CMySite::OnInPlaceDeactivate()
{
  return S_OK;
}

STDMETHODIMP CMySite::DiscardUndoState()
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMySite::DeactivateAndUndo()
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMySite::OnPosRectChange(LPCRECT lprcPosRect)
{
  return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// CMyFrame
//


// IUnknown
STDMETHODIMP CMyFrame::QueryInterface(REFIID riid,void ** ppvObject)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP_(ULONG) CMyFrame::AddRef(void)
{
  return 1;
}

STDMETHODIMP_(ULONG) CMyFrame::Release(void)
{
  return 1;
}

// IOleWindow
STDMETHODIMP CMyFrame::GetWindow(HWND FAR* lphwnd)
{
  *lphwnd = this->host->hwnd;
  return S_OK;
//  NOTIMPLEMENTED;
}

STDMETHODIMP CMyFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
  NOTIMPLEMENTED;
}

// IOleInPlaceUIWindow
STDMETHODIMP CMyFrame::GetBorder(LPRECT lprectBorder)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMyFrame::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMyFrame::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMyFrame::SetActiveObject(IOleInPlaceActiveObject *pActiveObject,LPCOLESTR pszObjName)
{
  return S_OK;
}

// IOleInPlaceFrame
STDMETHODIMP CMyFrame::InsertMenus(HMENU hmenuShared,LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMyFrame::SetMenu(HMENU hmenuShared,HOLEMENU holemenu,HWND hwndActiveObject)
{
  return S_OK;
}

STDMETHODIMP CMyFrame::RemoveMenus(HMENU hmenuShared)
{
  NOTIMPLEMENTED;
}

STDMETHODIMP CMyFrame::SetStatusText(LPCOLESTR pszStatusText)
{
  return S_OK;
}

STDMETHODIMP CMyFrame::EnableModeless(BOOL fEnable)
{
  return S_OK;
}

STDMETHODIMP CMyFrame::TranslateAccelerator(  LPMSG lpmsg,WORD wID)
{
  NOTIMPLEMENTED;
}



webhostwnd::webhostwnd()
{
  site.host = this;
  frame.host = this;
}

webhostwnd::~webhostwnd()
{
}

HWND webhostwnd::operator =(webhostwnd* rhs)
{
  return hwnd;
}

BOOL webhostwnd::HandleMessage(UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT* r)
{
  if(uMsg == WM_DESTROY)
  {
    UnCreateEmbeddedWebControl();
    return TRUE;
  }
  else if(uMsg == WM_CREATE)
  {
    webhostwnd* _this = (webhostwnd*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
    _this->CreateEmbeddedWebControl();
    return TRUE;
  }

  return FALSE;
}


void webhostwnd::CreateEmbeddedWebControl()
{
  OleCreate(CLSID_WebBrowser,IID_IOleObject,OLERENDER_DRAW,0,&site,&storage,(void**)&mpWebObject);

  mpWebObject->SetHostNames(L"Web Host",L"Web View");

  // I have no idea why this is necessary. remark it out and everything works perfectly.
  OleSetContainedObject(mpWebObject,TRUE);

  RECT rect;
  GetClientRect(hwnd,&rect);

  mpWebObject->DoVerb(OLEIVERB_SHOW,NULL,&site,-1,hwnd,&rect);

  IWebBrowser2* iBrowser;
  mpWebObject->QueryInterface(IID_IWebBrowser2,(void**)&iBrowser);

  VARIANT vURL;
  vURL.vt = VT_BSTR;
  vURL.bstrVal = SysAllocString(url);
  VARIANT ve1, ve2, ve3, ve4;
  ve1.vt = VT_EMPTY;
  ve2.vt = VT_EMPTY;
  ve3.vt = VT_EMPTY;
  ve4.vt = VT_EMPTY;

  iBrowser->put_Left(0);
  iBrowser->put_Top(0);
  iBrowser->put_Width(rect.right);
  iBrowser->put_Height(rect.bottom);

  iBrowser->Navigate2(&vURL, &ve1, &ve2, &ve3, &ve4);

  VariantClear(&vURL);

  iBrowser->Release();
}


void webhostwnd::UnCreateEmbeddedWebControl(void)
{
  mpWebObject->Close(OLECLOSE_NOSAVE);
  mpWebObject->Release();
}