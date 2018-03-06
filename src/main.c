#include "common.h"
#include "name.h"
#include "memory.h"
#include "log.h"
#include "data.h"

#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
//#include <ntifs.h>

DECLARE_PATH;

#ifdef UNICODE
#   define WPRTS L"s"
#else
#   define WPRTS L"hs"
#endif

bool long_path_check(DWORD *p_val, union message_error *p_error)
{
    bool succ = 0;
    HKEY key;
    LSTATUS stat;
    if ((stat = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\FileSystem"), 0, KEY_READ, &key)) != ERROR_SUCCESS) p_error->error_wapi = MESSAGE_ERROR_WAPI(stat);
    else
    {
        DWORD val_sz = sizeof(*p_val);
        if ((stat = RegQueryValueEx(key, _T("LongPathsEnabled"), NULL, NULL, (LPBYTE) p_val, &val_sz)) != ERROR_SUCCESS) p_error->error_wapi = MESSAGE_ERROR_WAPI(stat);
        else succ = 1;
        RegCloseKey(key);
    }
    return succ;
}

bool symlink_get_target(const TCHAR *file, TCHAR *buff, size_t *p_buff_cap, union message_error *p_error)
{
    bool succ = 0;
    HANDLE f;
    char buff_loc[sizeof(REPARSE_DATA_BUFFER) + (SHRT_MAX + 1) * (sizeof_field(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) << 1)];
    
    if ((f = CreateFile(file, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL)) == INVALID_HANDLE_VALUE) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
    else
    {
        DWORD buff_loc_sz = 0;
        if (!DeviceIoControl(f, FSCTL_GET_REPARSE_POINT, NULL, 0, buff_loc, sizeof(buff_loc), &buff_loc_sz, NULL)) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
        else
        {
            REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER *) buff_loc;
            DWORD off = rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(rdb->SymbolicLinkReparseBuffer.PathBuffer[0]);
            DWORD len = rdb->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(rdb->SymbolicLinkReparseBuffer.PathBuffer[0]);
                        
            if (_sntprintf(buff, *p_buff_cap, _T("%.*ls"), (int) len, rdb->SymbolicLinkReparseBuffer.PathBuffer + off) != (int) len) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
            else succ = 1;
            *p_buff_cap = len + 1;
        }
        CloseHandle(f);
    }
    return succ;
}

bool symlink_set_target(const TCHAR *file, TCHAR *path, size_t path_len, union message_error *p_error)
{
    bool succ = 0;
    char *buff_loc;
    if (!array_init_strict(&buff_loc, path_len, sizeof_field(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) << 1, sizeof(REPARSE_DATA_BUFFER), 1)) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
    else
    {
        size_t path_sz = path_len * sizeof_field(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer), tot_sz = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + (path_sz << 1);
        REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER *) buff_loc;

        rdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;
        rdb->ReparseDataLength = (USHORT) (tot_sz - offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer));
        rdb->Reserved = 0;
        rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
        rdb->SymbolicLinkReparseBuffer.SubstituteNameLength = (USHORT) path_sz;
        rdb->SymbolicLinkReparseBuffer.PrintNameOffset = (USHORT) path_sz;
        rdb->SymbolicLinkReparseBuffer.PrintNameLength = (USHORT) path_sz;
        rdb->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;

        // Note, that 'buff_loc' can actually contain 'path_len + 1' characters
        if (_snwprintf(rdb->SymbolicLinkReparseBuffer.PathBuffer, path_len + 1, L"%.*" WPRTS, (int) path_len, path) != path_len) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
        else
        {
            memcpy(rdb->SymbolicLinkReparseBuffer.PathBuffer + path_len, rdb->SymbolicLinkReparseBuffer.PathBuffer, path_sz);

            HANDLE f;
            if ((f = CreateFile(file, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL)) == INVALID_HANDLE_VALUE) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
            else
            {
                if (!DeviceIoControl(f, FSCTL_SET_REPARSE_POINT, buff_loc, (DWORD) tot_sz, NULL, 0, NULL, NULL)) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
                else succ = 1;
                CloseHandle(f);
            }
        }
        free(buff_loc);
    }
    return succ;
}

bool symlink_get_final_target(const TCHAR *file, TCHAR *buff, size_t *p_buff_cap, union message_error *p_error)
{
    bool succ = 0;
    HANDLE f;
    
    if ((f = CreateFile(file, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL)) == INVALID_HANDLE_VALUE) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
    else
    {
        size_t len, cap = *p_buff_cap;
        if (!(len = GetFinalPathNameByHandle(f, buff, (DWORD) cap, FILE_NAME_NORMALIZED)) || len > cap) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
        else succ = 1;
        *p_buff_cap = len + 1;
        CloseHandle(f);
    }
    return succ;
}

/*
bool folder_change_name(const TCHAR *path, DWORD height)
{
    return 0;
}
*/

bool symlink_check(const TCHAR *path, DWORD *p_res, union message_error *p_error)
{
    bool succ = 0;
    DWORD att;

    if ((att = GetFileAttributes(path)) == INVALID_FILE_ATTRIBUTES) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
    else *p_res = !!(att & FILE_ATTRIBUTE_REPARSE_POINT), succ = 1;
    
    return succ;
}

struct test_folder {
    size_t depth;
};

bool test_folder_enter(const TCHAR *path, size_t path_len, void *Context)
{
    struct test_folder *restrict context = Context;
    _tprintf(_T("Entering (depth: %zu): \"%.*s\".\n"), context->depth++, (int) path_len, path);
    return 1;
}

bool test_folder_exit(const TCHAR *path, size_t path_len, void *Context)
{
    struct test_folder *restrict context = Context;
    _tprintf(_T("Exiting  (depth: %zu): \"%.*s\".\n"), --context->depth, (int) path_len, path);
    return 1;
}

bool test_file(const TCHAR *path, size_t path_len, void *Context)
{
    struct test_folder *restrict context = Context;
    _tprintf(_T("*FILE*   (depth: %zu): \"%.*s\".\n"), context->depth, (int) path_len, path);
    return 1;
}

bool test_folder_error(const TCHAR *path, size_t path_len, void *Context)
{
    struct test_folder *restrict context = Context;
    _tprintf(_T("*ERROR*  (depth: %zu): \"%.*s\".\n"), context->depth, (int) path_len, path);
    return 1;
}

typedef bool (*traverse_callback)(const TCHAR *, size_t, void *);

struct traverse_events {
    traverse_callback folder_enter, folder_exit, folder_error, folder_reparse, file;
};

bool traverse_event_issue(traverse_callback event, TCHAR *buff, size_t buff_cnt, void *context)
{
    if (!event) return 1;

    TCHAR tmp = buff[buff_cnt];
    buff[buff_cnt] = _T('\0'); // Obtaining correct folder name
    bool succ = event(buff, buff_cnt, context);
    buff[buff_cnt] = tmp;
    return succ;
}

bool folder_traverse(const TCHAR *path, size_t path_len, struct traverse_events events, void *context, union message_error *p_error)
{
    enum pop { POP_NONE = 0, POP_COMPLETE, POP_LIGHWEIGHT };
    
    bool succ = 0;
    struct { size_t ptr, cnt, cap; struct { HANDLE fnd; size_t str_off; } *frm; } stk = { .cap = 1 };
    struct { TCHAR *buff; size_t cnt, cap; } str = { .cnt = path_len };
    const TCHAR pat[] = _T("\\*");

    if (!array_init(&stk.frm, &stk.cap, sizeof(*stk.frm), 0, 0)) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
    else
    {
        if (!array_resize(&str.buff, &str.cap, sizeof(*str.buff), 0, ARRAY_RESIZE_EXTEND_ONLY, ARGS(str.cnt, _countof(pat)))) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
        else
        {
            _tcsncpy(str.buff, path, str.cnt);
            _tcscpy(str.buff + str.cnt, pat);
            stk.frm[0].str_off = 0;

            for (;;)
            {
                enum pop pop = POP_NONE;

                WIN32_FIND_DATA dat;                
                if ((stk.frm[stk.ptr].fnd = FindFirstFile(str.buff, &dat)) == INVALID_HANDLE_VALUE)
                {
                    if (GetLastError() != ERROR_ACCESS_DENIED) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
                    else
                    {
                        succ = traverse_event_issue(events.folder_error, str.buff, str.cnt, context); // Access denied
                        if (succ) pop = POP_LIGHWEIGHT;
                    }
                }
                else
                {
                    stk.cnt++;
                    succ = traverse_event_issue(events.folder_enter, str.buff, str.cnt, context); // Enter the folder 
                }

                if (succ)
                {                   
                    succ = 0;
                    for (;;)
                    {
                        if (pop) // Pop from the stack
                        {
                            if (pop == POP_COMPLETE)
                            {
                                succ = traverse_event_issue(events.folder_exit, str.buff, str.cnt, context); // Exit the folder
                                FindClose(stk.frm[stk.ptr].fnd);
                                stk.cnt--;
                            }
                            else succ = 1;

                            pop = POP_NONE;
                            str.cnt = stk.frm[stk.ptr].str_off;
                            
                            if (stk.ptr)
                            {
                                _tcscpy(str.buff + str.cnt + 1, pat + 1);
                                stk.ptr--;
                            }
                            else break;
                        }
                        else
                        {
                            size_t diff = _tcslen(dat.cFileName) + 1; // Additional space is required for the '\\' symbol
                            if (!array_resize(&str.buff, &str.cap, sizeof(*str.buff), 0, ARRAY_RESIZE_EXTEND_ONLY, ARGS(diff, str.cnt))) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
                            else succ = 1;
                            
                            if (succ)
                            {
                                _tcsncpy(str.buff + str.cnt + 1, dat.cFileName, diff - 1);
                                str.cnt += diff;

                                if (dat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                {
                                    if (dat.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                                        succ = traverse_event_issue(events.folder_reparse, str.buff, str.cnt, context); // Reparse point
                                    else if (_tcscmp(dat.cFileName, _T("..")) && _tcscmp(dat.cFileName, _T("."))) // Push on the stack
                                    {
                                        if (stk.ptr == stk.cap - 1)
                                        {
                                            if (!array_resize(&stk.frm, &stk.cap, sizeof(*stk.frm), 0, ARRAY_RESIZE_EXTEND_ONLY, ARGS(stk.cap, 1))) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
                                            else succ = 1;
                                        }
                                        else succ = 1;

                                        if (succ)
                                        {
                                            succ = 0;
                                            if (!array_resize(&str.buff, &str.cap, sizeof(*str.buff), 0, ARRAY_RESIZE_EXTEND_ONLY, ARGS(str.cnt, _countof(pat)))) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
                                            else succ = 1;

                                            if (succ)
                                            {
                                                stk.ptr++;
                                                stk.frm[stk.ptr].str_off = str.cnt - diff;
                                                _tcscpy(str.buff + str.cnt, pat);
                                                break;
                                            }
                                        }
                                    }
                                    else succ = 1;
                                }
                                else succ = traverse_event_issue(events.file, str.buff, str.cnt, context); // File found

                                str.cnt -= diff;
                                _tcscpy(str.buff + str.cnt + 1, pat);                                
                            }
                        }

                        if (succ)
                        {
                            succ = 0;
                            DWORD stat;
                            if (!FindNextFile(stk.frm[stk.ptr].fnd, &dat)) if ((stat = GetLastError()) != ERROR_NO_MORE_FILES) p_error->error_wapi = MESSAGE_ERROR_WAPI(stat);
                            else
                            {
                                pop = POP_COMPLETE;
                                continue;
                            }
                            else continue;
                        }

                        break;
                    }
                }

                if (!succ)
                {
                    // Cleanup
                    while (stk.cnt) FindClose(stk.frm[--stk.cnt].fnd);
                    break;
                }
                else if (stk.ptr) succ = 0;
                else break;
            }
            free(str.buff);
        }        
        free(stk.frm);
    }

    return succ;
}

TCHAR *prepare_path(const TCHAR *path, size_t *p_cnt, union message_error *p_error)
{
    TCHAR *res = NULL;
    const TCHAR prefix[] = _T("\\\\?\\");
    size_t cnt;
    if ((cnt = GetFullPathName(path, 0, NULL, NULL)) == 0) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
    else
    {
        if (!array_resize_strict(&res, 0, sizeof(*res), 0, ARRAY_RESIZE_EXTEND_ONLY, ARGS(cnt, strlenof(prefix)))) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
        else
        {
            if (GetFullPathName(path, (DWORD) cnt, res + strlenof(prefix), NULL) != cnt - 1) p_error->error_crt = MESSAGE_ERROR_CRT(EINVAL);
            else
            {
                if (_tcsncmp(res, prefix, strlenof(prefix))) _tcsncpy(res, prefix, strlenof(prefix)), *p_cnt = cnt + strlenof(prefix);
                else memmove(res, res + strlenof(prefix), cnt * sizeof(*res)), *p_cnt = cnt;
                return res;
            }
        }        
    }
    return NULL;
}

struct dir_context {
    TCHAR *path;
};

bool proc_dir(TCHAR *glb_path, TCHAR *loc_path, TCHAR* loc_path_new, union message_error *p_error)
{
    bool succ = 0;
    TCHAR *path = NULL, *path_new = NULL;
    size_t glb_path_len = _tcslen(glb_path), loc_path_len = _tcslen(loc_path), loc_path_new_len = _tcslen(loc_path_new);

    if (!array_resize_strict(&path, 0, sizeof(path), 0, ARRAY_RESIZE_EXTEND_ONLY, ARGS(glb_path_len, loc_path_len, 1)) ||
        !array_resize_strict(&path_new, 0, sizeof(path_new), 0, ARRAY_RESIZE_EXTEND_ONLY, ARGS(glb_path_len, loc_path_new_len, 1))) p_error->error_crt = MESSAGE_ERROR_CRT(errno);
    else
    {      
        if (!MoveFile(path, path_new)) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
        {
            if (folder_traverse(glb_path, glb_path_len, (struct traverse_events) { .folder_enter = test_folder_enter, .folder_exit = test_folder_exit, .folder_error = test_folder_error, .file = test_file }, &(struct test_folder) { 0 }, p_error))
            {

            }
        }
    }

    free(path);
    free(path_new);
    return succ;
}

bool proc_symlink_chref(TCHAR *path, TCHAR *new_target, union message_error *p_error)
{
    bool succ = 0;
    DWORD sl = 0;
    if (!symlink_check(path, &sl, p_error)) return 0;

    if (sl)
    {
        if (symlink_set_target(path, new_target, _tcslen(new_target), p_error)) succ = 1;
    }
    else
        _tprintf(_T("\"%s\" is not a symlink."), path), succ = 1;

    return succ;
}

bool proc_symlink_info(TCHAR *path, union message_error *p_error)
{
    bool succ = 0;
    TCHAR buff[LARGE_BUFF];

    size_t path_new_cnt;
    TCHAR *path_new = prepare_path(path, &path_new_cnt, p_error);
    if (path_new)
    {
        DWORD sl = 0;
        if (!symlink_check(path_new, &sl, p_error)) return 0;

        if (sl)
        {
            size_t cnt0 = _countof(buff);
            if (symlink_get_target(path_new, buff, &cnt0, p_error))
            {
                size_t cnt1 = _countof(buff) - cnt0;
                if (symlink_get_final_target(path_new, buff + cnt0, &cnt1, p_error))
                {
                    _tprintf(_T("\"%s\" -> \"%.*s\" [\"%.*s\"]\n"), path, (int) cnt0, buff, (int) cnt1, buff + cnt0);
                    succ = 1;
                }
                else
                    _tprintf(_T("\"%s\" -> \"%.*s\n"), path, (int) cnt0, buff);
            }
        }
        else
            _tprintf(_T("\"%s\" is not a symlink."), path), succ = 1;
        free(path_new);
    }
    return succ;
}

bool proc_symlink_make(TCHAR *path, TCHAR *target, union message_error *p_error)
{
    bool succ = 0;
    size_t path_new_cnt;
    TCHAR *path_new = prepare_path(path, &path_new_cnt, p_error);
    if (path_new)
    {
        while (path_new[path_new_cnt - 2] == _T('\\')) path_new[path_new_cnt-- - 2] = _T('\0');
        if (!CreateSymbolicLink(path_new, target, SYMBOLIC_LINK_FLAG_DIRECTORY | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) p_error->error_wapi = MESSAGE_ERROR_WAPI(GetLastError());
        else succ = 1;
        free(path_new);
    }
    return succ;
}

int compare(const TCHAR **a, const TCHAR **b)
{
    return _tcscmp(*a, *b);
}

int _tmain(int argc, TCHAR *argv[])
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    
    bool succ = 0;
    struct log log;
    union message_error error = { 0 };

    if (log_init(&log, NULL, SMALL_BUFF))
    {
        DWORD en;
        if (!long_path_check(&en, &error)) log_message(&log, &error.base);
        else if (en) succ = 1;
        else log_close(&log);
    }
    
    if (!succ) return EXIT_FAILURE;
    
#   ifdef UNICODE
    _setmode(_fileno(stderr), _O_U16TEXT);
#   endif
        
    TCHAR *cmdv[] = { _T("chdir"), _T("chref"), _T("dir"), _T("mkref"), _T("ref") };
    enum { CMDI_CHDIR, CMDI_CHREF, CMDI_DIR, CMDI_MKREF, CMDI_REF };

    for (int i = 1; i < argc; i++)
    {
        TCHAR **res = bsearch(&argv[i], cmdv, _countof(cmdv), sizeof(*cmdv), compare);

        if (!res) continue;
        switch (res - cmdv)
        {
        case CMDI_CHDIR: // Three arguments: global path, local path, new local path
            if (i + 3 < argc)
            {
                if (!proc_dir(argv[i + 1], argv[i + 2], argv[i + 3], &error)) log_message(&log, &error.base);
                i += 3;
            }
            break;

        case CMDI_CHREF:
            if (i + 2 < argc) // Two arguments: path, new target
            {
                if (!proc_symlink_chref(argv[i + 1], argv[i + 2], &error)) log_message(&log, &error.base);
                i += 2;
            }
            break;

        case CMDI_DIR:
            if (i + 1 < argc)
            {
                if (!folder_traverse(argv[i + 1], _tcslen(argv[i + 1]), (struct traverse_events) { .folder_enter = test_folder_enter, .folder_exit = test_folder_exit, .folder_error = test_folder_error, .file = test_file }, &(struct test_folder) { 0 }, &error)) log_message(&log, &error.base);
                i++;
            }
            break;

        case CMDI_MKREF:
            if (i + 2 < argc)
            {
                if (!proc_symlink_make(argv[i + 1], argv[i + 2], &error)) log_message(&log, &error.base);
                i += 2;
            }
            break;

        case CMDI_REF:
            if (i + 1 < argc) 
            {
                if (!proc_symlink_info(argv[i + 1], &error)) log_message(&log, &error.base);
                i++;
            }
            break;
        }
    }

    log_close(&log);
    return EXIT_SUCCESS;
}