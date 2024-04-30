#pragma once
#include <string>

#include "NoCopyable.hpp"

namespace xs {

inline int FileRemove(const std::string& filename) {
    return std::remove(filename.c_str());
}

inline int FileRename(const std::string& filename1, const std::string& filename2) {
    return std::rename(filename1.c_str(), filename2.c_str());
}

inline bool IsDirectory(std::string path) {
#ifdef WIN32
    return PathIsDirectory(path.c_str()) ? true : false;
#else
    DIR* pdir = opendir(path.c_str());
    if (pdir != NULL) {
        closedir(pdir);
        pdir = NULL;
        return true;
    }
    return false;
#endif
}

inline int FileOpen(FILE** fp, const std::string& filename, const std::string& mode) {
#ifdef _WIN32
    *fp = _fsopen((filename.c_str()), mode.c_str(), _SH_DENYWR);
#else //unix
    *fp = fopen((filename.c_str()), mode.c_str());
#endif
    return *fp == nullptr;
}

//Return if file exists
inline bool FileExists(const std::string& filename) {
#ifdef _WIN32
    auto attribs = GetFileAttributes(filename.c_str());
    return (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY));
#else //common linux/unix all have the stat system call
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
#endif
}

inline size_t FileSize(FILE* f) {
    if (f == nullptr)
        return 0;
#ifdef _WIN32
    int fd = _fileno(f);
#if _WIN64 //64 bits
    struct _stat64 st;
    if (_fstat64(fd, &st) == 0)
        return st.st_size;

#else //windows 32 bits
    long ret = _filelength(fd);
    if (ret >= 0)
        return static_cast<size_t>(ret);
#endif

#else // unix
    int fd = fileno(f);
    //64 bits(but not in osx, where fstat64 is deprecated)
#if !defined(__FreeBSD__) && !defined(__APPLE__) && (defined(__x86_64__) || defined(__ppc64__))
    struct stat64 st;
    if (fstat64(fd, &st) == 0)
        return static_cast<size_t>(st.st_size);
#else // unix 32 bits or osx
    struct stat st;
    if (fstat(fd, &st) == 0)
        return static_cast<size_t>(st.st_size);
#endif
#endif
    return 0;
}

class FileHelper : public NoCopyable {
  public:
    const int open_tries = 5;
    const int open_interval = 10;

    explicit FileHelper() : _fd(nullptr) {}

    ~FileHelper() {
        Close();
    }

    void Open(const std::string& fname, bool truncate = false) {
        Close();
        auto* mode = truncate ? ("wb") : ("ab");
        _filename = fname;
        for (int tries = 0; tries < open_tries; ++tries) {
            if (!FileOpen(&_fd, fname, mode))
                return;
            std::this_thread::sleep_for(std::chrono::milliseconds(open_interval));
        }
    }

    void Reopen(bool truncate) {
        Open(_filename, truncate);
    }

    void Flush() {
        std::fflush(_fd);
    }

    void Close() {
        if (_fd) {
            std::fclose(_fd);
            _fd = nullptr;
        }
    }

    void Write(const std::string& msg) {
        if (!IsOpen()) {
            return;
        }
        size_t msg_size = msg.size();
        const char* data = msg.data();
        std::fwrite(data, 1, msg_size, _fd);
    }

    size_t Size() {
        return FileSize(_fd);
    }
    bool IsOpen() {
        return _fd != nullptr;
    }

  private:
    FILE* _fd = nullptr;
    std::string _filename;
};
}