#pragma once
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#endif

namespace TraProcessor {

class FileLockError : public std::runtime_error {
public:
    explicit FileLockError(const std::string& msg) : std::runtime_error(msg) {}
};

class FileLock {
private:
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;
#endif
    std::string lock_file_path;
    bool is_locked;

public:
    explicit FileLock(const std::string& file_path);
    ~FileLock();

    // 禁止拷贝和移动
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
    FileLock(FileLock&&) = delete;
    FileLock& operator=(FileLock&&) = delete;

    bool acquire(int timeout_ms = 3000);
    void release();
    bool isLocked() const { return is_locked; }
};

// RAII锁管理器
class FileGuard {
private:
    FileLock& lock;
    bool acquired;

public:
    explicit FileGuard(FileLock& l, int timeout_ms = 3000);
    ~FileGuard();

    bool isAcquired() const { return acquired; }

    // 禁止拷贝和移动
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;
    FileGuard(FileGuard&&) = delete;
    FileGuard& operator=(FileGuard&&) = delete;
};

} // namespace TraProcessor 