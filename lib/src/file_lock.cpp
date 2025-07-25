#include "file_lock.h"
#include <chrono>
#include <thread>
#include <system_error>

namespace TraProcessor {

FileLock::FileLock(const std::string& file_path) 
    : lock_file_path(file_path + ".lock"), is_locked(false) {
#ifdef _WIN32
    handle = INVALID_HANDLE_VALUE;
#else
    fd = -1;
#endif
}

FileLock::~FileLock() {
    release();
}

bool FileLock::acquire(int timeout_ms) {
    if (is_locked) return true;

    auto start = std::chrono::steady_clock::now();

    while (true) {
#ifdef _WIN32
        // Windows实现
        handle = CreateFileA(
            lock_file_path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,  // 不共享
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
            NULL
        );

        if (handle != INVALID_HANDLE_VALUE) {
            is_locked = true;
            return true;
        }

        DWORD error = GetLastError();
        if (error != ERROR_SHARING_VIOLATION && error != ERROR_FILE_EXISTS) {
            throw FileLockError("Failed to create lock file: " + std::to_string(error));
        }
#else
        // Linux实现
        fd = open(lock_file_path.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd != -1) {
            if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
                is_locked = true;
                return true;
            }
            close(fd);
            fd = -1;
        }
#endif

        // 检查超时
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeout_ms) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void FileLock::release() {
    if (!is_locked) return;

#ifdef _WIN32
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
#else
    if (fd != -1) {
        flock(fd, LOCK_UN);
        close(fd);
        unlink(lock_file_path.c_str());
        fd = -1;
    }
#endif

    is_locked = false;
}

// RAII锁管理器实现
FileGuard::FileGuard(FileLock& l, int timeout_ms) 
    : lock(l), acquired(false) {
    acquired = lock.acquire(timeout_ms);
    if (!acquired) {
        throw FileLockError("Failed to acquire file lock within timeout");
    }
}

FileGuard::~FileGuard() {
    if (acquired) {
        lock.release();
    }
}

} // namespace TraProcessor 