#include "mapped_file.hpp"

#include <iostream>
#include <system_error>
#include <cstring>

#include <sys/mman.h>
#include <sys/stat.h>

#include <unistd.h>

mapped_file::mapped_file(int fd) {
    struct stat st;
    if (fstat(fd, &st) != 0) {
        throw std::system_error(errno, std::generic_category(), "fstat");
    }

    _size = st.st_size;
    _addr = mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);

    if (_addr == MAP_FAILED) {
        throw std::system_error(errno, std::generic_category(), "mmap");
    }
}

size_t mapped_file::size() const {
    return _size;
}

mapped_file::~mapped_file() {
    if (_addr && _addr != MAP_FAILED && munmap(_addr, _size) != 0) {
        std::cerr << "munmap: " << errno << " - " << strerror(errno) << '\n';
        std::terminate();
    }
}
