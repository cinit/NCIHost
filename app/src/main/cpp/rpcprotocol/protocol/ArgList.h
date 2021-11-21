//
// Created by kinit on 2021-06-09.
//

#ifndef NCI_HOST_ARGLIST_H
#define NCI_HOST_ARGLIST_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <type_traits>

#include "../utils/HashMap.h"
#include "../utils/SharedBuffer.h"

namespace ipcprotocol {

class ArgList {
public:
    using uchar = unsigned char;

    /**
     * uchar type
     * | -7- + -6- + -5- + -4- + -3- + -2- + -1- + -0- |
     *   [7] RFU
     *        [6] is array
     *              [5 ... 4] type: map(3), number(2), string(1), raw(0)
     *              |
     *              + for type number(2)
     *              |           [3 ... 2] number length 64/32/16/8=3/2/1/0
     *              |                        [1] is bool or float or double
     *              |                              [0] signed or unsigned
     *              |
     *              + for type string(1)
     *              |           [3 ... 2] encoding UCS-32/UTF-16/UTF-8=2/1/0
     *              |                     currently only support UTF-8
     *              + for type raw(0)
     *              |                               [0] must be 1
     */
    class Types {
    private:
        constexpr static uchar T_ARRAY = 1u << 6u;
        constexpr static uchar T_STRING = 1u << 4u;
        constexpr static uchar T_RAW = 0u << 4u | 1u;
        constexpr static uchar T_MAP = 3u << 4u;
        constexpr static uchar L_FLOAT = 2u << 2u;
        constexpr static uchar L_DOUBLE = 3u << 2u;
        constexpr static uchar F_BOOLEAN = 1u << 1u;
        constexpr static uchar F_DECIMAL = 1u << 1u;
    public:
        constexpr static uchar F_SIGNED = 0u;
        constexpr static uchar F_UNSIGNED = 1u;
        constexpr static uchar L_8 = 0u << 2u;
        constexpr static uchar L_16 = 1u << 2u;
        constexpr static uchar L_32 = 2u << 2u;
        constexpr static uchar L_64 = 3u << 2u;
        constexpr static uchar T_NUMBER = 2u << 4u;
        constexpr static uchar MASK_ARRAY = T_ARRAY;
        constexpr static uchar TYPE_BOOLEAN = T_NUMBER | L_8 | F_BOOLEAN;
        constexpr static uchar TYPE_FLOAT = T_NUMBER | L_FLOAT | F_DECIMAL;
        constexpr static uchar TYPE_DOUBLE = T_NUMBER | L_DOUBLE | F_DECIMAL;
        constexpr static uchar TYPE_STRING = T_STRING;
        constexpr static uchar TYPE_MAP = T_MAP;
        constexpr static uchar TYPE_RAW = T_RAW;
        constexpr static uchar TYPE_INVALID = 0u;

        template<typename T>
        constexpr static uchar getTypeId() {
            static_assert(std::is_same<T, const std::string &>() || std::is_same<T, std::string>()
                          || std::is_same<T, const char *>() || std::is_same<T, char *>()
                          || std::is_same<T, bool>() || std::is_same<T, float>() || std::is_same<T, double>()
                          || (std::is_integral<T>() && !std::is_array<T>()), "unsupported type");
            if (std::is_same<T, const std::string &>() || std::is_same<T, std::string>()
                || std::is_same<T, const char *>() || std::is_same<T, char *>()) {
                return TYPE_STRING;
            } else if (std::is_same<T, bool>()) {
                return TYPE_BOOLEAN;
            } else if (std::is_same<T, float>()) {
                return TYPE_FLOAT;
            } else if (std::is_same<T, double>()) {
                return TYPE_DOUBLE;
            } else if (std::is_integral<T>() && !std::is_array<T>()) {
                uchar sign = std::is_unsigned<T>() ? F_UNSIGNED : F_SIGNED;
                uchar len = L_64;
                if (sizeof(T) == 1) {
                    len = L_8;
                } else if (sizeof(T) == 2) {
                    len = L_16;
                } else if (sizeof(T) == 4) {
                    len = L_32;
                }
                return T_NUMBER | len | sign;
            } else {
                return 0;
            }
        }

        template<class T>
        constexpr static uchar getTypeId(T) { return getTypeId<T>(); }

        constexpr static bool isDouble(uchar type) {
            return type == TYPE_DOUBLE;
        }

        constexpr static bool isFloat(uchar type) {
            return type == TYPE_FLOAT;
        }

        constexpr static bool isString(uchar type) {
            return type == TYPE_STRING;
        }
    };

    class Builder {
    private:
        int mCount = 0;
        HashMap<int, uchar> mArgTypes;
        HashMap<int, uint64_t> mArgInlineVars;
        HashMap<int, SharedBuffer> mArgBuffers;

        void pushRawInline(uchar type, uint64_t value);

        void pushRawBuffer(uchar type, const void *buffer, size_t size);

        void pushRawInlineArray(uchar type, const uint64_t *values, int count);

//        void pushRawBufferArray(uchar type, void *buffer, size_t *size, int count);

//        void pushRawBufferArray(uchar type, const std::vector<SharedBuffer> &buffers);

        static inline void extractStringBuffer(const char *in, const char **out, size_t *size) {
            if (in == nullptr) {
                *out = nullptr;
                *size = 0;
            } else {
                *out = in;
                *size = strlen(in);
            }
        }

        static inline void extractStringBuffer(const std::string &in, const char **out, size_t *size) {
            if (in.empty()) {
                *out = nullptr;
                *size = 0;
            } else {
                *out = in.c_str();
                *size = in.length();
            }
        }

    public:
        ArgList::Builder &push(const std::string &value);

        ArgList::Builder &push(const std::vector<char> &value);

        ArgList::Builder &push(const char *value);

        template<class T>
        ArgList::Builder &push(T value) {
            static_assert(std::is_same<T, bool>() || std::is_same<T, float>() || std::is_same<T, double>()
                          || (std::is_integral<T>() && !std::is_array<T>()), "not primitive");
            if (std::is_same<T, bool>()) {
                pushRawInline(Types::TYPE_BOOLEAN, value ? 1 : 0);
            } else if (std::is_same<T, float>()) {
                uint64_t v = 0;
                *reinterpret_cast<T *>(&v) = value;
                pushRawInline(Types::TYPE_FLOAT, v);
            } else if (std::is_same<T, double>()) {
                uint64_t v = 0;
                *reinterpret_cast<T *>(&v) = value;
                pushRawInline(Types::TYPE_DOUBLE, v);
            } else {
                uint64_t v = 0;
                *reinterpret_cast<T *>(&v) = value;
                pushRawInline(Types::getTypeId<T>(), v);
            }
            return *this;
        }

        template<class T0, class...Tp>
        inline ArgList::Builder &pushArgs(T0 v0, Tp...vp) {
            push(v0);
            pushArgs(vp...);
            return *this;
        }

        inline ArgList::Builder &pushArgs() {
            return *this;
        }

        template<class T>
        inline ArgList::Builder &operator<<(const T &value) { return push(value); }

        [[nodiscard]] SharedBuffer build() const;

        [[nodiscard]] inline int count() const noexcept {
            return mCount;
        }

        void reset();
    };

private:
    const void *mBuffer = nullptr;
    size_t mLength = 0;
    bool mIsValid = false;
    int mCount = 0;
    size_t mRegOffset = 0;
    size_t mPoolOffset = 0;

    bool readRawInlineValue(uint64_t *out, int index) const noexcept;

public:
    ArgList();

    ArgList(const void *buffer, size_t size);

    [[nodiscard]] inline int count() const noexcept {
        return mCount;
    }

    [[nodiscard]] inline bool isValid() const noexcept {
        return mIsValid;
    }

    [[nodiscard]] inline const void *getBuffer() const noexcept {
        return mBuffer;
    }

    [[nodiscard]] inline size_t getBufferSize() const noexcept {
        return mLength;
    }

    template<class T, typename Condition=std::enable_if_t<
            (std::is_same<T, bool>() || std::is_same<T, float>() || std::is_same<T, double>() ||
             (std::is_integral<T>() && !std::is_array<T>())) && sizeof(T) <= 8, T>>
    [[nodiscard]] inline bool get(T *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (Types::getTypeId<T>() != *(reinterpret_cast<const uchar *>(mBuffer) + 8 + index)) {
            return false;
        }
        *out = *reinterpret_cast<T *>(&reg);
        return true;
    }

    template<class T=std::string>
    [[nodiscard]] inline bool get(std::string *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (Types::getTypeId<std::string>() != *(reinterpret_cast<const uchar *>(mBuffer) + 8 + index)) {
            return false;
        }
        struct BufferEntry {
            uint32_t offset;
            uint32_t length;// in bytes
        };
        static_assert(sizeof(BufferEntry) == 8);
        const BufferEntry *entry = reinterpret_cast<BufferEntry *>(&reg);// ub
        auto offset = entry->offset;
        auto len = entry->length;
        if (offset + len > mLength) {
            return false;
        }
        *out = std::string(reinterpret_cast<const char *>(mBuffer) + offset, len);
        return true;
    }

    template<class T=std::vector<char>>
    [[nodiscard]] inline bool get(std::vector<char> *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (Types::TYPE_RAW != *(reinterpret_cast<const uchar *>(mBuffer) + 8 + index)) {
            return false;
        }
        struct BufferEntry {
            uint32_t offset;
            uint32_t length;// in bytes
        };
        static_assert(sizeof(BufferEntry) == 8);
        const BufferEntry *entry = reinterpret_cast<BufferEntry *>(&reg);// ub
        auto offset = entry->offset;
        auto len = entry->length;
        if (offset + len > mLength) {
            return false;
        }
        *out->resize(len);
        memcpy(out->data(), reinterpret_cast<const char *>(mBuffer) + offset, len);
        return true;
    }

    [[nodiscard]] inline uchar getType(int index) const {
        if (!mIsValid || index < 0 || index >= mCount) {
            return Types::TYPE_INVALID;
        }
        return *(reinterpret_cast<const uchar *>(mBuffer) + 8 + index);
    }
};

}

#endif //NCI_HOST_ARGLIST_H
