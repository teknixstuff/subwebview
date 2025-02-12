/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

// include BaseUtil.h instead of including directly

// auto-free memory for arbitrary malloc()ed memory of type T*
template <typename T>
class ScopedMem
{
    T *obj;
public:
    ScopedMem() : obj(NULL) {}
    explicit ScopedMem(T* obj) : obj(obj) {}
    ~ScopedMem() { free(obj); }
    void Set(T *o) {
        free(obj);
        obj = o;
    }
    T *Get() const { return obj; }
    T *StealData() {
        T *tmp = obj;
        obj = NULL;
        return tmp;
    }
    operator T*() const { return obj; }
};

class ScopedCritSec
{
    CRITICAL_SECTION *cs;
public:
    explicit ScopedCritSec(CRITICAL_SECTION *cs) : cs(cs) {
        EnterCriticalSection(cs);
    }
    ~ScopedCritSec() {
        LeaveCriticalSection(cs);
    }
};

class ScopedHandle {
    HANDLE handle;
public:
    explicit ScopedHandle(HANDLE handle) : handle(handle) { }
    ~ScopedHandle() { CloseHandle(handle); }
    operator HANDLE() const { return handle; }
};

// deletes any object at the end of the scope
template <class T>
class ScopedPtr
{
    T *obj;
public:
    ScopedPtr() : obj(NULL) {}
    explicit ScopedPtr(T* obj) : obj(obj) {}
    ~ScopedPtr() { delete obj; }
    T *Detach() {
        T *tmp = obj;
        obj = NULL;
        return tmp;
    }
    operator T*() const { return obj; }
    T* operator->() const { return obj; }
    T* operator=(T* newObj) {
        delete obj;
        return (obj = newObj);
    }
};

template <class T>
class ScopedComPtr {
protected:
    T* ptr;
public:
    ScopedComPtr() : ptr(NULL) { }
    explicit ScopedComPtr(T *ptr) : ptr(ptr) { }
    ~ScopedComPtr() {
        if (ptr)
            ptr->Release();
    }
    bool Create(const CLSID clsid) {
        CrashIf(ptr);
        if (ptr) return false;
        HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_PPV_ARGS(&ptr));
        return SUCCEEDED(hr);
    }
    operator T*() const { return ptr; }
    T** operator&() { return &ptr; }
    T* operator->() const { return ptr; }
    T* operator=(T* newPtr) {
        if (ptr)
            ptr->Release();
        return (ptr = newPtr);
    }
};

template <class T>
class ScopedComQIPtr : public ScopedComPtr<T> {
public:
    ScopedComQIPtr() : ScopedComPtr() { }
    explicit ScopedComQIPtr(IUnknown *unk) {
        HRESULT hr = unk->QueryInterface(&this->ptr);
        if (FAILED(hr))
            this->ptr = NULL;
    }
    T* operator=(IUnknown *newUnk) {
        if (this->ptr)
            this->ptr->Release();
        HRESULT hr = newUnk->QueryInterface(&this->ptr);
        if (FAILED(hr))
            this->ptr = NULL;
        return this->ptr;
    }
};

template <typename T>
class ScopedGdiObj {
    T obj;
public:
    explicit ScopedGdiObj(T obj) : obj(obj) { }
    ~ScopedGdiObj() { DeleteObject(obj); }
    operator T() const { return obj; }
};
typedef ScopedGdiObj<HFONT> ScopedFont;

class ScopedCom {
public:
    ScopedCom() { CoInitialize(NULL); }
    ~ScopedCom() { CoUninitialize(); }
};

class ScopedOle {
public:
    ScopedOle() { OleInitialize(NULL); }
    ~ScopedOle() { OleUninitialize(); }
};

class ScopedGdiPlus {
protected:
    Gdiplus::GdiplusStartupInput si;
    Gdiplus::GdiplusStartupOutput so;
    ULONG_PTR token, hookToken;
    bool noBgThread;

public:
    // suppress the GDI+ background thread when initiating in WinMain,
    // as that thread causes DDE messages to be sent too early and
    // thus causes unexpected timeouts
    explicit ScopedGdiPlus(bool inWinMain=false) : noBgThread(inWinMain) {
        si.SuppressBackgroundThread = noBgThread;
        Gdiplus::GdiplusStartup(&token, &si, &so);
        if (noBgThread)
            so.NotificationHook(&hookToken);
    }
    ~ScopedGdiPlus() {
        if (noBgThread)
            so.NotificationUnhook(hookToken);
        Gdiplus::GdiplusShutdown(token);
    }
};
