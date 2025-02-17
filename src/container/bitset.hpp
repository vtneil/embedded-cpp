#ifndef LIB_XCORE_CONTAINER_BITSET_HPP
#define LIB_XCORE_CONTAINER_BITSET_HPP

#include "internal/macros.hpp"
#include "memory.hpp"
#include <cstdint>

LIB_XCORE_BEGIN_NAMESPACE

namespace container {
  template<size_t Nb, typename WordT = int, template<typename, size_t> class Container = array_t>
  struct bitset_t {
  private:
    using byte_t                              = signed char;

    static constexpr size_t       Alignment   = sizeof(WordT);
    static constexpr size_t       NumBytes    = (Nb + 8 - 1) / 8;
    static constexpr size_t       SizeActual  = nearest_alignment<byte_t, Alignment>(NumBytes);
    static constexpr size_t       NumElements = SizeActual / Alignment;

    Container<WordT, NumElements> data_       = {};

    // Read-write bit reference
    class bit_reference {
      bitset_t &parent_;
      size_t    index_;

    public:
      bit_reference(bitset_t &parent, const size_t index)
          : parent_(parent), index_(index) {}

      operator bool() const {  // Implicit
        return parent_.get(index_);
      }

      bit_reference &operator=(const bool value) {
        parent_.set(index_, value);
        return *this;
      }

      bit_reference &operator=(const bit_reference &other) {
        return *this = static_cast<bool>(other);
      }

      bit_reference &operator&=(const bool value) {
        const bool current = parent_.get(index_);
        parent_.set(index_, current & value);
        return *this;
      }

      // Bitwise OR assignment
      bit_reference &operator|=(const bool value) {
        const bool current = parent_.get(index_);
        parent_.set(index_, current | value);
        return *this;
      }

      // Bitwise XOR assignment (toggle)
      bit_reference &operator^=(const bool value) {
        const bool current = parent_.get(index_);
        parent_.set(index_, current ^ value);
        return *this;
      }
    };

    // Read-only bit reference
    class const_bit_reference {
      const bitset_t &parent_;
      size_t          index_;

    public:
      const_bit_reference(const bitset_t &parent, const size_t index)
          : parent_(parent), index_(index) {}

      operator bool() const {  // Implicit
        return parent_.get(index_);
      }
    };

  public:
    // Constructor

    constexpr bitset_t()                      = default;

    constexpr bitset_t(const bitset_t &other) = default;

    constexpr bitset_t(bitset_t &&other)      = default;

    // Methods

    size_t find_first_true() {
      for (size_t i = 0; i < NumElements; ++i) {
        if (!data_[i])
          continue;
        for (size_t j = 0; j < 8 * Alignment; ++j) {
          if ((data_[i] >> j) & 1)
            return i * Alignment + j;
        }
      }
      return size();
    }

    size_t find_first_false() {
      for (size_t i = 0; i < NumElements; ++i) {
        if (!~data_[i])
          continue;
        for (size_t j = 0; j < 8 * Alignment; ++j) {
          if (!((data_[i] >> j) & 1))
            return i * Alignment + j;
        }
      }
      return size();
    }

    [[nodiscard]] constexpr bool get(const size_t index) const {
      size_t idx_word = index / (8 * Alignment);
      size_t idx_bit  = index % (8 * Alignment);
      return (data_[idx_word] >> idx_bit) & 1;
    }

    [[nodiscard]] uint64_t get(const size_t from, const size_t to) const {
      uint64_t result = 0;
      size_t   shift  = 0;

      for (size_t i = from; i < to; ++i, ++shift) {
        result |= (static_cast<uint64_t>(get(i)) << shift);
      }
      return result;
    }

    constexpr void set(const size_t index, bool value) {
      size_t idx_word = index / (8 * Alignment);
      size_t idx_bit  = index % (8 * Alignment);

      data_[idx_word] &= ~(static_cast<WordT>(1) << idx_bit);
      data_[idx_word] |= static_cast<WordT>(value) << idx_bit;
    }

    template<typename T>
    void set(const size_t from, const size_t to, T value) {
      size_t start_word = from / (8 * Alignment);
      size_t end_word   = to / (8 * Alignment);
      size_t start_bit  = from % (8 * Alignment);
      size_t end_bit    = to % (8 * Alignment);

      if (start_word == end_word) {
        WordT mask        = ((static_cast<WordT>(1) << (end_bit - start_bit)) - 1) << start_bit;
        data_[start_word] = (data_[start_word] & ~mask) | ((value << start_bit) & mask);
      } else {
        // First partial word
        if (start_bit != 0) {
          WordT mask        = (~static_cast<WordT>(0)) << start_bit;
          data_[start_word] = (data_[start_word] & ~mask) | ((value << start_bit) & mask);
          value >>= (8 * Alignment - start_bit);
          ++start_word;
        }

        // Full words
        for (size_t i = start_word; i < end_word; ++i) {
          data_[i] = static_cast<WordT>(value);
          value >>= (8 * Alignment);
        }

        // Last partial word
        if (end_bit != 0) {
          WordT mask      = (static_cast<WordT>(1) << end_bit) - 1;
          data_[end_word] = (data_[end_word] & ~mask) | (value & mask);
        }
      }
    }

    bit_reference operator[](const size_t index) {
      return bit_reference(*this, index);
    }

    const_bit_reference operator[](const size_t index) const {
      return const_bit_reference(*this, index);
    }

    constexpr void clear(const size_t index) {
      size_t idx_word = index / (8 * Alignment);
      size_t idx_bit  = index % (8 * Alignment);

      data_[idx_word] &= ~(static_cast<WordT>(1) << idx_bit);
    }

    constexpr void toggle(const size_t index) {
      size_t idx_word = index / (8 * Alignment);
      size_t idx_bit  = index % (8 * Alignment);

      data_[idx_word] ^= static_cast<WordT>(1) << idx_bit;
    }

    void clear_all() {
      memset(data_, 0x00, SizeActual);
    }

    void set_all() {
      memset(data_, 0xFF, SizeActual);
    }

    bitset_t &operator&=(const bitset_t &other) {
      for (size_t i = 0; i < NumElements; ++i) {
        data_[i] &= other.data_[i];
      }
      return *this;
    }

    bitset_t &operator|=(const bitset_t &other) {
      for (size_t i = 0; i < NumElements; ++i) {
        data_[i] |= other.data_[i];
      }
      return *this;
    }

    bitset_t operator&(const bitset_t &other) const {
      bitset_t dst{*this};
      dst &= other;
      return dst;
    }

    bitset_t operator|(const bitset_t &other) const {
      bitset_t dst{*this};
      dst |= other;
      return dst;
    }

    bool operator==(const bitset_t &other) const {
      for (size_t i = 0; i < NumElements; ++i) {
        if (data_[i] != other.data_[i]) return false;
      }
      return true;
    }

    bool operator!=(const bitset_t &other) const {
      return !(this->operator==(other));
    }

    // Capacity

    [[nodiscard]] constexpr size_t size() const {
      return Nb;
    }

    [[nodiscard]] constexpr size_t capacity() const {
      return Nb;
    }

    // Explicit conversions

    template<typename TargetT>
    [[nodiscard]] FORCE_INLINE constexpr explicit operator TargetT *() noexcept {
      return reinterpret_cast<TargetT *>(static_cast<WordT *>(data_));
    }

    template<typename TargetT>
    [[nodiscard]] FORCE_INLINE constexpr explicit operator const TargetT *() const noexcept {
      return reinterpret_cast<const TargetT *>(static_cast<const WordT *>(data_));
    }

    constexpr auto as_bytes() -> unsigned char * {
      return static_cast<unsigned char *>(*this);
    }

    [[nodiscard]] constexpr auto as_bytes() const -> const unsigned char * {
      return static_cast<const unsigned char *>(*this);
    }

    template<typename TargetT>
    [[nodiscard]] FORCE_INLINE constexpr auto ptr(const size_t byte_offset) -> TargetT * {
      return reinterpret_cast<TargetT *>(this->as_bytes() + byte_offset);
    }

    template<typename TargetT>
    [[nodiscard]] FORCE_INLINE constexpr auto ptr(const size_t byte_offset) const -> const TargetT * {
      return reinterpret_cast<const TargetT *>(this->as_bytes() + byte_offset);
    }
  };
}  // namespace container


using namespace container;

LIB_XCORE_END_NAMESPACE

#endif  //LIB_XCORE_CONTAINER_BITSET_HPP
