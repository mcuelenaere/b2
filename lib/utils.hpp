#ifndef __UTILS_HPP_
#define __UTILS_HPP_

#include <stdio.h>
#include <string.h>

#include <memory>
#include <stdexcept>
#include <sstream>

namespace b2 {

template<typename T>
inline void free_deleter(T* ptr) noexcept
{
    free((void*) ptr);
}

template<typename T>
std::unique_ptr<T, decltype(&free_deleter<T>)> make_unique_ptr_with_free_deleter(T* ptr)
{
    return std::unique_ptr<T, decltype(&free_deleter<T>)>(ptr, &free_deleter<T>);
}

template<typename T>
using unique_ptr_with_free_deleter = std::unique_ptr<T, decltype(&free_deleter<T>)>;

struct FileGuard {
    FILE* fd;

    FileGuard(const char* filename, const char* mode) {
        fd = fopen(filename, mode);
        if (!fd) {
            std::stringstream ss;
            ss << "Couldn't open '" << filename << "': " << strerror(errno);
            throw std::runtime_error(ss.str());
        }
    }

    ~FileGuard() {
        if (fclose(fd) != 0) {
            std::stringstream ss;
            ss << "Couldn't close file: " << strerror(errno);
            throw std::runtime_error(ss.str());
        }
    }
};

} // namespace b2

#endif // __UTILS_HPP_
