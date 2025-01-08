#pragma once

#include <cstring>
#include <span>
#include <string>

namespace secure_cleanse_impl {

// Pointer to memset is volatile so that compiler must de-reference
// the pointer and can't assume that it points to any function in
// particular (such as memset, which it then might further "optimize")
typedef void* (*memset_t)(void*, int, size_t);
static volatile memset_t memset_func = memset;

}

inline void secure_cleanse(void *ptr, size_t len);
inline void secure_cleanse(std::span<std::byte> target);
inline void secure_cleanse(std::string &target);

inline void secure_cleanse(void *ptr, size_t len) {
  /// Securely erase a block of memory
  secure_cleanse_impl::memset_func(ptr, 0, len);
}

inline void secure_cleanse(std::span<std::byte> target) {
  /// Securely erase a block of memory pointed to by a span
  secure_cleanse_impl::memset_func(target.data(), 0, target.size());
}

inline void secure_cleanse(std::string &target) {
  /// Securely erase a string
  target.resize(target.capacity(), '\0');
  secure_cleanse_impl::memset_func(target.data(), 0, target.size());
  target.clear();
}
