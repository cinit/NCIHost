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

#include "HashMap.h"
#include "SharedBuffer.h"

class ArgList {
public:
    using uchar = unsigned char;

    class Types {
    private:
        constexpr static uchar T_ARRAY = 1u << 6u;
        constexpr static uchar T_STRING = 1u << 4u;
        constexpr static uchar T_RAW = 2u << 4u;
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
        constexpr static uchar T_NUMBER = 0u << 4u;
        constexpr static uchar MASK_ARRAY = T_ARRAY;
        constexpr static uchar TYPE_BOOLEAN = T_NUMBER | L_8 | F_BOOLEAN;
        constexpr static uchar TYPE_FLOAT = T_NUMBER | L_FLOAT | F_DECIMAL;
        constexpr static uchar TYPE_DOUBLE = T_NUMBER | L_DOUBLE | F_DECIMAL;
        constexpr static uchar TYPE_STRING = T_STRING;
        constexpr static uchar TYPE_MAP = T_MAP;
        constexpr static uchar TYPE_RAW = T_RAW;

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
                return 0xFF;
            }
        }

        template<class T>
        constexpr static uchar getTypeId(T) { return getTypeId<T>(); }
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
        ArgList::Builder &push(const std::string &value) {
            const char *buffer = nullptr;
            size_t size = 0;
            extractStringBuffer(value, &buffer, &size);
            if (buffer == nullptr) {
                pushRawInline(Types::TYPE_STRING, 0);
            } else {
                pushRawBuffer(Types::TYPE_STRING, (void *) buffer, size + 1);
            }
            return *this;
        }

        ArgList::Builder &push(const char *value) {
            const char *buffer = nullptr;
            size_t size = 0;
            extractStringBuffer(value, &buffer, &size);
            if (buffer == nullptr) {
                pushRawInline(Types::TYPE_STRING, 0);
            } else {
                pushRawBuffer(Types::TYPE_STRING, (void *) buffer, size + 1);
            }
            return *this;
        }

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
                uchar sign = std::is_unsigned<T>() ? Types::F_UNSIGNED : Types::F_SIGNED;
                uchar len;
                static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "size error");
                if (sizeof(T) == 1) {
                    len = Types::L_8;
                } else if (sizeof(T) == 2) {
                    len = Types::L_16;
                } else if (sizeof(T) == 4) {
                    len = Types::L_32;
                } else {
                    len = Types::L_64;
                }
                uchar type = Types::T_NUMBER | len | sign;
                uint64_t v = 0;
                *reinterpret_cast<T *>(&v) = value;
                pushRawInline(type, v);
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

        [[nodiscard]] SharedBuffer build();

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

    template<class T>
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

    [[nodiscard]] inline bool get(std::string *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (Types::getTypeId<std::string>() != *(reinterpret_cast<const uchar *>(mBuffer) + 8 + index)) {
            return false;
        }
        auto offset = (uint32_t) reg;
        auto len = (uint32_t) (reg >> 32u);
        if (offset + len > mLength) {
            return false;
        }
        *out = std::string(reinterpret_cast<const char *>(mBuffer ) + offset, len);
        return true;
    }
};


#endif //NCI_HOST_ARGLIST_H
