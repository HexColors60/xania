#pragma once

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace test {

class MemFile {
    FILE *file_;
    char *ptr_;
    size_t size_;

public:
    explicit MemFile(std::string_view contents) : file_(open_memstream(&ptr_, &size_)) {
        ::fwrite(contents.data(), 1, contents.size(), file_);
        ::fseek(file_, 0, SEEK_SET);
    }
    ~MemFile() {
        ::fclose(file_);
        ::free(ptr_);
    }
    MemFile(const MemFile &) = delete;
    MemFile &operator=(const MemFile &) = delete;
    // MemFile can't be moved (even though the file_ could) as the memstream has references to
    // values inside the MemFile (size_ and _ptr).
    MemFile(MemFile &&) = delete;
    MemFile &operator=(MemFile &&) = delete;

    FILE *file() const noexcept { return file_; }
};

}