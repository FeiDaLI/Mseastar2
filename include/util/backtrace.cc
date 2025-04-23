#include "backtrace.hh"
#include <functional>

size_t saved_backtrace::hash() const {
    size_t h = 0;
    for (auto frame : _frames) {
        h = std::hash<unw_word_t>()(frame) + (h << 6) + (h << 16) - h;
    }
    return h;
}

saved_backtrace current_backtrace() {
    std::vector<unw_word_t> v;
    backtrace([&] (uintptr_t addr) {
        v.push_back(addr);
    });
    return saved_backtrace(std::move(v));
} 