//
// Created by kinit on 2021-06-13.
//

#ifndef RPCPROTOCOL_CONCURRENTHASHMAP_H
#define RPCPROTOCOL_CONCURRENTHASHMAP_H

#include <memory>
#include <utility>
#include <set>
#include <mutex>
#include <unordered_map>

template<typename K, typename V, typename Hash = std::hash<K>, typename Pred = std::equal_to<K>>
class ConcurrentHashMap {
public:
    class Entry {
    public:
        Entry(const K &k, const V &v) : key(k), value(v) {}

        Entry(const Entry &) = delete;

        Entry &operator=(const Entry &) = delete;

        [[nodiscard]] K getKey() const {
            return key;
        }

        [[nodiscard]] V *getValue() const {
            return &value;
        }

        template<typename... Args>
        void setValue(Args &&...args) const {
            value = V(std::forward<Args>(args)...);
        }

        void setValue(V &v) const {
            value = v;
        }

    private:
        const K key;
        mutable V value;
    };

private:
    std::unordered_map<K, std::shared_ptr<Entry>, Hash, Pred> backend;
    mutable std::mutex mutex;
public:
    ConcurrentHashMap() = default;

    template<typename AnyMap>
    explicit ConcurrentHashMap(const AnyMap &map) {
        const auto entries = map.entrySet();
        for (const auto &entry: entries) {
            backend.insert_or_assign(entry->getKey(), std::make_shared<Entry>
                    (entry->getKey(), *entry->getValue()));
        }
    }

    [[nodiscard]] std::set<std::shared_ptr<Entry>> entrySet() const {
        std::scoped_lock<std::mutex> _(mutex);
        std::set<std::shared_ptr<Entry>> copy = std::set<std::shared_ptr<Entry>>();
        for (const auto &entry: backend) {
            copy.emplace(entry.second);
        }
        return copy;
    }

    [[nodiscard]] size_t size() const {
        std::scoped_lock<std::mutex> _(mutex);
        return backend.size();
    }

    [[nodiscard]] bool isEmpty() const {
        return size() == 0;
    }

    [[nodiscard]] bool containsKey(const K &key) const {
        std::scoped_lock<std::mutex> _(mutex);
        return backend.find(key) != backend.end();
    }

    [[nodiscard]] V *get(const K &key) const {
        std::scoped_lock<std::mutex> _(mutex);
        auto p = backend.find(key);
        if (p != backend.end()) {
            return p->second.get()->getValue();
        } else {
            return nullptr;
        }
    }

    template<typename... Args>
    void put(const K &key, Args &&...args) {
        auto entry = std::make_shared<Entry>(key, V(std::forward<Args>(args)...));
        std::scoped_lock<std::mutex> _(mutex);
        backend.insert_or_assign(key, entry);
    }

    template<typename... Args>
    bool putIfAbsent(const K &key, Args &&...args) {
        auto entry = std::make_shared<Entry>(key, V(std::forward<Args>(args)...));
        std::scoped_lock<std::mutex> _(mutex);
        auto result = backend.try_emplace(key, entry);
        return result.second;
    }

    bool remove(const K &key) {
        std::scoped_lock<std::mutex> _(mutex);
        return backend.erase(key) != 0;
    }

    void clear() {
        std::scoped_lock<std::mutex> _(mutex);
        backend.clear();
    }

    template<typename AnyMap>
    void putAll(const AnyMap &map) {
        const auto entries = map.entrySet();
        std::scoped_lock<std::mutex> _(mutex);
        for (const auto &entry: entries) {
            backend.insert_or_assign(entry->getKey(), std::make_shared<Entry>
                    (entry->getKey(), *entry->getValue()));
        }
    }
};

#endif //RPCPROTOCOL_CONCURRENTHASHMAP_H
