#include "filesys.h"
#include <cstring>

#ifdef _WIN32
# include <direct.h>
#else
# include <sys/stat.h>
#endif

namespace {

namespace sys {

inline int mkdir(const char * dirName)
{
#ifdef _WIN32
    return _mkdir(dirName);
#else
    return ::mkdir(dirName, 0755);
#endif
}

} // namespace sys

} // namespace

class FileSys::Impl final:
        public Logger
{
public:
    auto mkdir(std::string dirName) -> bool
    {
        if(dirName.empty()) {
            msg(LogLevel::error, "Attempt to create directory with empty name");
            return false;
        }
        if(sys::mkdir(dirName.data())) {
            if(errno == ENOENT) {
                size_t pos = dirName.find_last_of("/\\");
                if(pos != std::string::npos && pos > 0) {
                    return mkdir(dirName.substr(0, pos)) && mkdir(dirName);
                }
            }
            if(errno != EEXIST) {
                msg(LogLevel::error, "Could not create directory \"%s\": %s",
                    dirName.data(), strerror(errno));
                return false;
            }
        }
        return true;
    }
};

FileSys::FileSys():
    impl(std::make_unique<Impl>())
{
    captureLog(impl.get());
}

FileSys::~FileSys()
{
}

auto FileSys::mkdir(std::string dirName) -> bool
{
    return impl->mkdir(dirName);
}
