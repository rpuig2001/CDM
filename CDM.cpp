// rStatus.cpp : Defines the initialization routines for the DLL.
//
#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)
#include "CDMSingle.hpp"

extern "C" IMAGE_DOS_HEADER __ImageBase;

CDM* gpMyPlugin = NULL;
static PVOID g_vectoredHandle = nullptr;

LONG WINAPI VectoredHandler(EXCEPTION_POINTERS* pExceptionInfo);
static std::string GetTimestamp();
static std::string GetPluginFolder();
static HANDLE CreateDumpFile(std::string& outPath);

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance) {
    g_vectoredHandle = AddVectoredExceptionHandler(1, VectoredHandler);
    // create the instance
    *ppPlugInInstance = gpMyPlugin = new CDM();
}

//---EuroScopePlugInExit-----------------------------------------------

void __declspec(dllexport) EuroScopePlugInExit(void) {
    if (g_vectoredHandle) {
        RemoveVectoredExceptionHandler(g_vectoredHandle);
        g_vectoredHandle = nullptr;
    }
    // delete the instance
    delete gpMyPlugin;
}

LONG WINAPI VectoredHandler(EXCEPTION_POINTERS* pExceptionInfo) {
    DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;

    if (code == 0xE06D7363 ||  // C++ throw
        code == 0x40010006 ||  // OutputDebugString
        code == 0x406D1388 ||  // Thread naming
        code == 0x80000003 ||  // Breakpoint
        code == 0x80000004)    // Single step
        return EXCEPTION_CONTINUE_SEARCH;

    static bool handled = false;
    if (handled) return EXCEPTION_CONTINUE_SEARCH;
    handled = true;

    std::string dumpPath;
    HANDLE hFile = CreateDumpFile(dumpPath);

    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei{};
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pExceptionInfo;
        mdei.ClientPointers = FALSE;

        BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                                    static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs |           // global variables
                                                               MiniDumpWithProcessThreadData |  // thread stacks
                                                               MiniDumpWithHandleData |         // open handles
                                                               MiniDumpWithThreadInfo           // thread timing info
                                                               ),
                                    &mdei, nullptr, nullptr);

        CloseHandle(hFile);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static std::string GetTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

static std::string GetPluginFolder() {
    char path[MAX_PATH];
    GetModuleFileNameA((HMODULE)&__ImageBase, path, MAX_PATH);

    std::string folder(path);
    size_t pos = folder.find_last_of("\\/");
    if (pos != std::string::npos) folder = folder.substr(0, pos);

    return folder;
}

static HANDLE CreateDumpFile(std::string& outPath) {
    std::string timestamp = GetTimestamp();

    // 1. Try plugin folder
    char dllPath[_MAX_PATH];
    GetModuleFileNameA(HINSTANCE(&__ImageBase), dllPath, sizeof(dllPath));
    string fld = dllPath;
    fld.resize(fld.size() - strlen("CDM.dll"));
    std::string file = fld + "\\CDM_" + timestamp + ".dmp";

    HANDLE hFile = CreateFileA(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        outPath = file;
        return hFile;
    }

    // 2. Fallback → TEMP
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);

    file = std::string(tempPath) + "VCP_" + timestamp + ".dmp";

    hFile = CreateFileA(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        outPath = file;
        return hFile;
    }

    return INVALID_HANDLE_VALUE;
}