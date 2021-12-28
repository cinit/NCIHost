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
    /**
     * *** uint32_t TypeId overview:
     * uint8_t[3] complex type info, 0 if not complex, MSB
     * uint8_t[2] index type, 0 if not used
     * uint8_t[1] aux info, 0 if not used
     * uint8_t[0] major type, LSB
     * *** immediate major types: integer, float, double, bool, null
     *      uint8_t[0] major type:
     *      [6] always 1
     *      [5] always 0, not buffer-pool type
     *      [3] 0: float, double, bool, null
     *          [2 ... 1] null(0), bool(1), float(2), double(3)
     *      [3] 1: integer
     *          [2 ... 1] length: 3/2/1/0=[us]int(64/32/16/8)_t
     *          [0] signed(0), unsigned(1)
     * *** buffer-pool major type: string, raw buffer, POD structure data
     *      [6] always 1
     *      [5] always 1, is buffer-pool type
     *      [4] 0: string
     *          [3 ... 2] encoding: UTF-8(0), UTF-16(1), UCS-32(2)
     *          [1] endianness: little(0), big(1)
     *          [0] reserved, always 0
     *          Note that the string is not null-terminated.
     *          Currently, only UTF-8 is supported.
     *      [4] 1: raw buffer, serialized buffer or POD structure data
     *          [3] 0: raw/serialized buffer data
     *              [2] 0: raw buffer data
     *                  1: serialized buffer data
     *          [3] 1: POD structure data, sizeof(T) is stored in [10 ... 0]{uint8_t[1], uint8_t[0][2 ... 0]},
     *                  11 bits, max struct size is 2047 bytes
     * *** complex types:
     * 0: not a complex type
     * non-zero: complex type
     * [6] always 1 for complex types
     * [5 ... 2] always 0, reserved
     * [1] 0: array or set
     *      [0] 0: array, 1: set
     * [1] 1: map, index type is stored in uint8_t[2] index type, which must not be POD-structure data,
     *          value type is stored in uint8_t[0] major type
     * *** buffer layout:
     * ArgListBuffer {
     *     +0: uint32_t sizeof(ArgListBuffer),
     *     +4: uint32_t argc,
     *     +8: uint32_t[argc] argument types, aligned to 8 bytes,
     *     +immediateValueStart: uint64_t[argc] immediate argument values,
     *     +bufferPoolStart: uint8_t[][] buffer pools, each buffer pool is aligned to 8 bytes,
     * }
     * where immediateValueStart is the start of immediate values, bufferPoolStart is the start of buffer pools.
     * immediateValueStart = 8 + (4 * argc + 7) / 8 * 8,
     * bufferPoolStart = immediateValueStart + 8 * argc,
     * every entry start offset is aligned to 8 bytes.
     * *** reference to a buffer pool
     * BufferEntry {
     *     +0: uint32_t offset,
     *     +4: uint32_t length,
     * }
     * BufferEntry is stored in uint64_t immediate values as its native byte order(usually little endian).
     */
    class Types {
    private:
        template<typename T>
        struct is_serializable {
        private:
            using yes = std::true_type;
            using no = std::false_type;

            template<typename U>
            static auto test(int) -> decltype(std::declval<U>().deserializeFromByteVector(std::vector<uint8_t>())
                                              && std::vector<uint8_t>(std::declval<U>().serializeToByteVector())
                                                      .empty(), yes());

            template<typename>
            static no test(...);

        public:
            static constexpr bool value = std::is_same_v<decltype(test<T>(0)), yes>;
        };

    public:
        template<typename T>
        constexpr static bool is_serializable_v = is_serializable<T>::value;
    private:
        constexpr static uint32_t F_VALID = 1u << 6u;
        constexpr static uint32_t F_IMMEDIATE = F_VALID | 0u << 5u;
        constexpr static uint32_t F_BUFFER_POOL = F_VALID | 1u << 5u;
        constexpr static uint32_t T_NULL = F_IMMEDIATE | 0u << 1u;
        constexpr static uint32_t T_BOOL = F_IMMEDIATE | 1u << 1u;
        constexpr static uint32_t T_FLOAT = F_IMMEDIATE | 2u << 1u;
        constexpr static uint32_t T_DOUBLE = F_IMMEDIATE | 3u << 1u;
        constexpr static uint32_t T_INTEGER_BASE = F_IMMEDIATE | 3u << 1u;
        constexpr static uint32_t L_8 = 0u << 1u;
        constexpr static uint32_t L_16 = 1u << 1u;
        constexpr static uint32_t L_32 = 2u << 1u;
        constexpr static uint32_t L_64 = 3u << 1u;
        constexpr static uint32_t F_SIGNED = 0u;
        constexpr static uint32_t F_UNSIGNED = 1u;
        constexpr static uint32_t T_STRING_BASE = F_BUFFER_POOL | 0u << 4u;
        constexpr static uint32_t F_STRING_UTF8 = 0u << 2u; // UTF-8
        constexpr static uint32_t T_RAW_BUFFER = F_BUFFER_POOL | 1u << 4u;
        constexpr static uint32_t T_SERIALIZED_BUFFER = F_BUFFER_POOL | 1u << 4u | 1u << 2u;
        constexpr static uint32_t T_STRUCTURE_BASE = F_BUFFER_POOL | 1u << 4u | 1u << 3u;
        constexpr static uint32_t SHIFT_MAJOR = 0u;
        constexpr static uint32_t SHIFT_AUX = 8u;
        constexpr static uint32_t SHIFT_INDEX = 16u;
        constexpr static uint32_t SHIFT_COMPLEX = 24u;
    public:
        constexpr static uint32_t TYPE_BOOLEAN = T_BOOL;
        constexpr static uint32_t TYPE_FLOAT = T_FLOAT;
        constexpr static uint32_t TYPE_DOUBLE = T_DOUBLE;
        constexpr static uint32_t TYPE_STRING = T_STRING_BASE | F_STRING_UTF8;
        constexpr static uint32_t TYPE_BYTE_BUFFER = T_RAW_BUFFER;
        constexpr static uint32_t TYPE_SERIALIZED_BUFFER = T_SERIALIZED_BUFFER;
        constexpr static uint32_t TYPE_INVALID = 0u;

        template<typename T>
        constexpr static uint32_t getPrimitiveTypeId() {
            if constexpr (std::is_same<T, const std::string &>() || std::is_same<T, std::string>()
                          || std::is_same<T, const char *>() || std::is_same<T, char *>()) {
                return TYPE_STRING;
            } else if constexpr(std::is_same<T, bool>()) {
                return TYPE_BOOLEAN;
            } else if constexpr(std::is_same<T, float>()) {
                return TYPE_FLOAT;
            } else if constexpr(std::is_same<T, double>()) {
                return TYPE_DOUBLE;
            } else if constexpr(std::is_integral<T>() && !std::is_array<T>()) {
                // integer
                return T_INTEGER_BASE | (std::is_signed<T>() ? F_SIGNED : F_UNSIGNED)
                       | (sizeof(T) == 1 ? L_8 : 0u)
                       | (sizeof(T) == 2 ? L_16 : 0u)
                       | (sizeof(T) == 4 ? L_32 : 0u)
                       | (sizeof(T) == 8 ? L_64 : 0u);
            } else if constexpr(std::is_same_v<T, std::vector<uint8_t>> || std::is_same_v<T, SharedBuffer>) {
                // byte buffer
                return TYPE_BYTE_BUFFER;
            } else if constexpr (std::is_class_v<T> && std::is_default_constructible_v<T> && is_serializable_v<T>) {
                // class and POD structure may use its own serialization
                return TYPE_SERIALIZED_BUFFER;
            } else if constexpr(std::is_pod_v<T> && !std::is_array_v<T> && sizeof(T) < 2048) {
                // POD structure buffer, max size 2047 bytes
                constexpr size_t structSize = sizeof(T);
                constexpr uint32_t high8bits = (structSize >> 3u) & 0xFFu;
                constexpr uint32_t low3bits = structSize & 7u;
                return T_STRUCTURE_BASE | (high8bits << SHIFT_AUX) | (low3bits << SHIFT_MAJOR);
            } else {
                // unsupported type
                return TYPE_INVALID;
            }
        }

        template<typename T>
        constexpr static uint32_t getTypeId() {
            // TODO: support complex type
            return getPrimitiveTypeId<T>();
        }

        template<class T>
        constexpr static uint32_t getTypeId(T) { return getTypeId<T>(); }

        constexpr static bool isValidType(uint32_t typeId) {
            return typeId != 0u;
        }

        /**
         * Immediate types, byte buffer and structure are primitive types.
         * Map, array, set, etc. are not primitive types.
         * @param type
         * @return is type primitive
         */
        constexpr static bool isPrimitiveType(uint32_t type) {
            return isValidType(type) && (type >> SHIFT_COMPLEX) == 0;
        }

        constexpr static bool isDouble(uint32_t type) {
            return type == TYPE_DOUBLE;
        }

        constexpr static bool isFloat(uint32_t type) {
            return type == TYPE_FLOAT;
        }

        constexpr static bool isString(uint32_t type) {
            return type == TYPE_STRING;
        }

        constexpr static bool isByteBuffer(uint32_t type) {
            return type == TYPE_BYTE_BUFFER;
        }

        constexpr static bool isSerializedBuffer(uint32_t type) {
            return type == TYPE_SERIALIZED_BUFFER;
        }

        constexpr static bool isStructure(uint32_t type) {
            return isPrimitiveType(type) && (
                    (type & (T_STRUCTURE_BASE | TYPE_BYTE_BUFFER | T_SERIALIZED_BUFFER
                             | T_STRING_BASE)) == T_STRUCTURE_BASE);
        }

        constexpr static bool isImmediateValue(uint32_t type) {
            return isPrimitiveType(type) && ((type & (F_IMMEDIATE | F_BUFFER_POOL)) == F_IMMEDIATE);
        }

        // complex types
    private:
        constexpr static uint32_t F_COMPLEX = 1u << (SHIFT_COMPLEX + 6u);
        constexpr static uint32_t C_ARRAY = F_COMPLEX;

    public:

        template<uint32_t BaseType>
        constexpr static std::enable_if_t<isPrimitiveType(BaseType), uint32_t> TYPE_ARRAY_OF = BaseType | C_ARRAY;

    };

    class Builder {
    private:
        int mCount = 0;
        HashMap<int, uint32_t> mArgTypes;
        HashMap<int, uint64_t> mArgInlineVars;
        HashMap<int, SharedBuffer> mArgBuffers;

        void pushRawInline(uint32_t type, uint64_t value);

        void pushRawBuffer(uint32_t type, const void *buffer, size_t size);

        void pushRawInlineArray(uint32_t type, const uint64_t *values, int count);

//        void pushRawBufferArray(uint32_t type, void *buffer, size_t *size, int count);

//        void pushRawBufferArray(uint32_t type, const std::vector<SharedBuffer> &buffers);

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

        ArgList::Builder &push(const std::vector<uint8_t> &value);

        ArgList::Builder &push(const SharedBuffer &value);

        ArgList::Builder &push(const char *value);

        template<class T, uint32_t TypeId = Types::getTypeId<T>(), typename CheckType=std::enable_if_t<
                Types::isImmediateValue(TypeId), T>>
        ArgList::Builder &push(T value) {
            // immediate value
            pushRawInline(TypeId, value);
            return *this;
        }

        template<class T, uint32_t TypeId = Types::getTypeId<T>(), typename CheckType=std::enable_if_t<
                Types::isStructure(TypeId) || Types::isSerializedBuffer(TypeId), T>>
        ArgList::Builder &push(const T &value) {
            if constexpr(Types::isStructure(TypeId)) {
                // structure data
                pushRawBuffer(TypeId, &value, sizeof(T));
            } else if constexpr(Types::isSerializedBuffer(TypeId)) {
                // serialized buffer
                std::vector<uint8_t> buffer = value.serializeToByteVector();
                pushRawBuffer(TypeId, buffer.data(), buffer.size());
            } else {
                // should not happen
                abort();
            }
            return *this;
        }

        template<typename BaseType, uint32_t BaseTypeId = Types::getPrimitiveTypeId<BaseType>(),
                uint32_t ComplexTypeId = Types::TYPE_ARRAY_OF<BaseTypeId>, typename ComplexType = std::vector<BaseType>,
                typename CheckType = std::enable_if_t<!std::is_same_v<BaseType, uint8_t>
                                                      && Types::isPrimitiveType(BaseTypeId), ComplexType>>
        ArgList::Builder &push(const std::vector<BaseType> &value) {
            SharedBuffer sb;
            sb.ensureCapacity(8);
            struct ElementIndex {
                uint32_t offset; // from the start of the buffer
                uint32_t length; // in bytes
            };
            struct ArrayBufferHeader {
                uint32_t count;
                uint32_t rfu;
                // ElementIndex index[]
            };
            static_assert(sizeof(ArrayBufferHeader) == 8);
            static_assert(sizeof(ElementIndex) == 8);
            std::vector<std::vector<uint8_t>> buffers;
            for (int i = 0; i < value.size(); ++i) {
                const BaseType &v = value[i];
                if constexpr(Types::isImmediateValue(BaseTypeId)) {
                    // immediate value
                    uint64_t tmp = 0;
                    memcpy(&tmp, &v, sizeof(BaseType));
                    buffers.emplace_back(reinterpret_cast<const uint8_t *>(&tmp), 8);
                } else if constexpr(Types::isStructure(BaseTypeId)) {
                    // structure
                    buffers.emplace_back(reinterpret_cast<const uint8_t *>(&v), sizeof(BaseType));
                } else if constexpr(Types::isString(BaseTypeId)) {
                    // string
                    std::string str = v;
                    const auto *first = reinterpret_cast<const uint8_t *>(str.c_str());
                    const auto *last = first + str.length();
                    buffers.emplace_back(first, last);
                } else if constexpr(Types::isByteBuffer(BaseTypeId)) {
                    // byte buffer
                    const std::vector<uint8_t> &ref = v;
                    buffers.emplace_back(ref.data(), ref.size());
                } else if constexpr(Types::isSerializedBuffer(BaseTypeId)) {
                    // serialized buffer
                    std::vector<uint8_t> buffer = v.serializeToByteVector();
                    buffers.emplace_back(buffer.data(), buffer.size());
                } else {
                    // should not be here
                    abort();
                }
            }
            sb.ensureCapacity(8 + buffers.size() * 8);
            memset(sb.get(), 0, 8 + 8 * buffers.size());
            *sb.at<ArrayBufferHeader>(0) = {uint32_t(value.size()), 0};
            ElementIndex *index = sb.at<ElementIndex>(8);
            for (int i = 0; i < buffers.size(); ++i) {
                index[i] = {0, uint32_t(buffers[i].size())}; // offset will be set later
            }
            // arrange buffers in the order of increasing offset
            size_t currentOffset = 8 + 8 * buffers.size();
            for (int i = 0; i < buffers.size(); ++i) {
                const std::vector<uint8_t> &buffer = buffers[i];
                index[i].offset = currentOffset;
                size_t alignedSize = std::max((buffer.size() + 7u) & ~7u, size_t(8u));
                sb.ensureCapacity(currentOffset + alignedSize);
                // reassign the index pointer due to the reallocation
                index = sb.at<ElementIndex>(8);
                if (alignedSize != buffer.size()) {
                    // fill the rest padding with zeros
                    memset(sb.at<uint8_t>(0) + currentOffset + buffer.size(), 0, alignedSize - buffer.size());
                }
                memcpy(sb.at<uint8_t>(0) + currentOffset, buffer.data(), buffer.size());
                currentOffset += alignedSize;
            }
            pushRawBuffer(ComplexTypeId, sb.get(), currentOffset);
            return *this;
        }

        template<class T0, class...Tp>
        inline ArgList::Builder &pushArgs(T0 &&v0, Tp &&...vp) {
            push(std::forward<T0>(v0));
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

    [[nodiscard]] inline uint32_t getType(int index) const {
        if (!mIsValid || index < 0 || index >= mCount) {
            return Types::TYPE_INVALID;
        }
        return *reinterpret_cast<const uint32_t *>((const uint8_t *) mBuffer + 8 + index * 4);
    }

    /**
     * Read the value of the argument at the given index.
     * This method will return false if the index is out of bounds.
     * @tparam T type of value
     * @tparam TypeId type id of value
     * @tparam CheckType check whether type id is correct
     * @param out the result
     * @param index the argument index
     * @return true if success, false if failed
     */
    template<class T, uint32_t TypeId = Types::getTypeId<T>(), typename CheckType=std::enable_if_t<
            Types::isImmediateValue(TypeId) || Types::isStructure(TypeId) || Types::isSerializedBuffer(TypeId), T>>
    [[nodiscard]] inline bool get(T *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (TypeId != getType(index)) {
            return false;
        }
        if constexpr(Types::isImmediateValue(TypeId)) {
            *out = *reinterpret_cast<T *>(&reg);
        } else {
            // buffer types
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
            if constexpr(Types::isStructure(TypeId)) {
                // structure
                *out = *reinterpret_cast<const T *>(reinterpret_cast<const char *>(mBuffer) + offset);
            } else if constexpr(Types::isSerializedBuffer(TypeId)) {
                // serialized buffer
                std::vector<uint8_t> buffer = {reinterpret_cast<const uint8_t *>(mBuffer) + offset,
                                               reinterpret_cast<const uint8_t *>(mBuffer) + offset + len};
                return out->deserializeFromByteVector(buffer);
            } else {
                // should not happen
                abort();
            }
        }
        return true;
    }

    template<class T=std::string>
    [[nodiscard]] inline bool get(std::string *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (Types::TYPE_STRING != getType(index)) {
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

    template<class T=std::vector<uint8_t>>
    [[nodiscard]] inline bool get(std::vector<uint8_t> *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (Types::TYPE_BYTE_BUFFER != getType(index)) {
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
        out->resize(len);
        memcpy(out->data(), reinterpret_cast<const char *>(mBuffer) + offset, len);
        return true;
    }

    template<typename BaseType, uint32_t BaseTypeId = Types::getPrimitiveTypeId<BaseType>(),
            uint32_t ComplexTypeId = Types::TYPE_ARRAY_OF<BaseTypeId>, typename ComplexType = std::vector<BaseType>,
            typename CheckType = std::enable_if_t<!std::is_same_v<BaseType, uint8_t>
                                                  && Types::isPrimitiveType(BaseTypeId), ComplexType>>
    [[nodiscard]] inline bool get(std::vector<BaseType> *out, int index) const {
        uint64_t reg = 0;
        if (!readRawInlineValue(&reg, index)) {
            return false;
        }
        if (Types::TYPE_ARRAY_OF<BaseTypeId> != getType(index)) {
            return false;
        }
        struct BufferEntry {
            uint32_t offset;
            uint32_t length; // in bytes
        };
        static_assert(sizeof(BufferEntry) == 8);
        const BufferEntry *pBufferEntry = reinterpret_cast<BufferEntry *>(&reg); // ub
        auto bufferOffset = pBufferEntry->offset;
        auto bufferLength = pBufferEntry->length;
        if (bufferOffset + bufferLength > mLength || bufferLength < 8) {
            return false;
        }
        struct ElementIndex {
            uint32_t offset; // from the start of the buffer
            uint32_t length; // in bytes
        };
        struct ArrayBufferHeader {
            uint32_t count;
            uint32_t rfu;
            // ElementIndex index[]
        };
        static_assert(sizeof(ArrayBufferHeader) == 8);
        static_assert(sizeof(ElementIndex) == 8);
        const auto *header = reinterpret_cast<const ArrayBufferHeader *>(((const uint8_t *) mBuffer) + bufferOffset);
        const auto *indexTable = reinterpret_cast<const ElementIndex *>(((const uint8_t *) mBuffer)
                                                                        + bufferOffset + sizeof(ArrayBufferHeader));
        out->resize(header->count);
        for (uint32_t i = 0; i < header->count; ++i) {
            const auto &elementEntry = indexTable[i];
            if (elementEntry.offset + elementEntry.length > bufferLength) {
                return false;
            }
            if constexpr(Types::isImmediateValue(BaseTypeId)) {
                // immediate value
                if (elementEntry.length != 8) {
                    return false;
                }
                uint64_t tmp = 0;
                memcpy(&tmp, ((const uint8_t *) mBuffer) + bufferOffset + elementEntry.offset, 8);
                out->at(i) = *reinterpret_cast<const BaseType *>(&tmp);
            } else if constexpr(Types::isStructure(BaseTypeId)) {
                // structure
                out->at(i) = *reinterpret_cast<const BaseType *>(((const uint8_t *) mBuffer) + bufferOffset +
                                                                 elementEntry.offset);
            } else if constexpr(Types::isString(BaseTypeId)) {
                // string
                out->at(i) = std::string((const char *) (((const uint8_t *) mBuffer)
                                                         + bufferOffset + elementEntry.offset), elementEntry.length);
            } else if constexpr(Types::isByteBuffer(BaseTypeId)) {
                // byte buffer
                std::vector<uint8_t> &ref = out->at(i);
                ref.resize(elementEntry.length);
                memcpy(ref.data(), ((const uint8_t *) mBuffer) + bufferOffset + elementEntry.offset,
                       elementEntry.length);
            } else if constexpr(Types::isSerializedBuffer(BaseTypeId)) {
                // serialized buffer
                std::vector<uint8_t> buffer = {
                        reinterpret_cast<const uint8_t *>(mBuffer) + bufferOffset + elementEntry.offset,
                        reinterpret_cast<const uint8_t *>(mBuffer) + bufferOffset + elementEntry.offset +
                        elementEntry.length};
                if (!out->at(i).deserializeFromByteVector(buffer)) {
                    return false;
                }
            } else {
                // should not be here
                return false;
            }
        }
        return true;
    }
};

}

#endif //NCI_HOST_ARGLIST_H
