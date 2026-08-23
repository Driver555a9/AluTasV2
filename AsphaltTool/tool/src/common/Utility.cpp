#include "Utility.h"

#include <filesystem>

#ifdef _WIN32 
    #include <windows.h>
#endif 

namespace AsphaltTas
{
    namespace Utility
    {
        bool SoftDelete(std::string_view path) noexcept
        {
            namespace fs = std::filesystem;

            std::error_code ec;
            fs::path abs_path = fs::absolute(path, ec);
            if (ec || !fs::exists(abs_path, ec)) 
            {
                return false;
            }
            
        #ifdef _WIN32 
            std::wstring wpath = abs_path.wstring();
            wpath.push_back(L'\0');

            SHFILEOPSTRUCTW fileOp = {};
            fileOp.wFunc = FO_DELETE;
            fileOp.pFrom = wpath.c_str();
            fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

            return SHFileOperationW(&fileOp) == 0;
        #else 
            fs::remove(abs_path);
            return true;
        #endif
        }
    }

}