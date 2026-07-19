/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/**
 * defines atomic_map and atomic_set.
 * This is a nice lightweight atomic set when not much else is needed.
 *
 * 2020-07-06 - slg - Upgraded to to C++17.
 */

#ifndef ATOMIC_MAP_H
#define ATOMIC_MAP_H

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

/*
 * Sample usage:
 * struct {int a, int b, int c} mycounter_t;
 * atomic_map<key, mycounter_t>.
 * Creates a defaultdict, and automatically cleans up memory.
 * Could be reimplemented to use smart pointers.
 */

template <class T1, class T2> class atomic_map {
    // T1 - key. For example, std::string
    // T2 - value. Should be a pointer.
    // Mutex M protects mymap.
    // It is mutable to allow modification in const methods
    mutable std::mutex M{};
    std::map<T1, T2 *> mymap{};

public:
    atomic_map() {}
    ~atomic_map() {
        /* delete everything in the map. Could do this with a unique_ptr? */
        clear();
    }
    class KeyError : public std::exception {
        T1 key;
    public:
        KeyError(T1 key_) : key(key_) {}
        const char* what() const noexcept override { return "did not convert key_ to a string"; }
    };
    /*
     * Create the behavior of a Python defaultdict:
     * If the object is not in the map, add it.
     * then return a reference to the object that is in the map.
     */
    T2 &operator[](const T1& key) {
        const std::lock_guard<std::mutex> lock(M);
        auto it = mymap.find(key);
        if (it == mymap.end()) {
            mymap[key] = new T2();
            return *(mymap[key]);
        }
        return *(it->second);
    }
    /*
     * Get behavior throws a key error if not present, and is const.
     */
    T2 &get(const T1& key) const {
        const std::lock_guard<std::mutex> lock(M);
        auto it = mymap.find(key);
        if (it == mymap.end()) {
            throw KeyError(key);
        }
        return *(it->second);
    }
    /*
     * insert. We want this in some cases. Fail if it already exists
     */
    void insert(const T1 &key, T2 *value) {
        const std::lock_guard<std::mutex> lock(M);
        auto it = mymap.find(key);
        if (it != mymap.end()) {
            throw KeyError(key);
        }
        mymap[key] = value;
    }

    /* We can't just pass-through to find, because we need to lock the mutext */
    typename std::map<T1, T2 *>::const_iterator find(const T1& key) const {
        const std::lock_guard<std::mutex> lock(M);
        return mymap.find(key);
    }
    /* We can't allow iteration through the map, since that would not be threadsafe, but we can allow the caller to get end(). */
#if 0
    typename std::map<T1, T2 *>::const_iterator begin() const {
        const std::lock_guard<std::mutex> lock(M);
        return mymap.begin();
    }
#endif
    typename std::map<T1, T2 *>::const_iterator end() const {
        const std::lock_guard<std::mutex> lock(M);
        return mymap.end();
    }

    void clear() {
        /* First delete all of the elements, then clear the map.  This
         * might be better done with unique_ptr(). However, then we
         * couldn't return a pointer, so we would need to use
         * shared_ptr(), which would incur a higher cost.
         */
        for (const auto &it : mymap) {
            delete it.second;
        }
        mymap.clear();
    }
    /* Number of elements */
    size_t size() const {
        const std::lock_guard<std::mutex> lock(M);
        return mymap.size();
    }
    /* implement this later */
    /* bytes */
    size_t bytes() const {
        const std::lock_guard<std::mutex> lock(M);
        size_t count = sizeof(*this);
        for (const auto &it : mymap) {
            count += sizeof(it.first) + sizeof(it.second) + it.first.size() + it.second->bytes();
        }
        return count;
    }

    bool contains(T1 key) const {
        const std::lock_guard<std::mutex> lock(M);
        return mymap.find(key) != mymap.end();
    }
    /* Like python .keys() */
    typename std::vector<T1> keys() const {
        const std::lock_guard<std::mutex> lock(M);
        std::vector<T1>ret;
        for (const auto &it : mymap) {
            ret.push_back( it.first );
        }
        return ret;
    }

    /* This is only threadsafe if the it.second is an object, and not a pointer*/
    /* Like python .values(). It should actually return objects. */
    typename std::vector<T2 *> values() const {
        const std::lock_guard<std::mutex> lock(M);
        std::vector<T2 *>ret;
        for (const auto &it : mymap) {
            ret.push_back( it.second );
        }
        return ret;
    }


    /* like Python .items() */
    /* This is used for dumping the contents in a mostly threadsafe manner.
     * The item that is return is a reference to what's in the atomic_map, so it better not be deleted,
     * and if you want multiple threads to access it, the elements should be atomic.
     * There is no reference counting on the pointer, so be careful!
     * It would be useful to have a priority queue to get the topN.
     */
    struct item {
        item(const item& s): key(s.key), value(s.value){};
        item(item &&that) noexcept : key(that.key), value(that.value) {}
        item& operator=(const item& s) { this->key = s.key; this->value = s.value; return *this;}

        item(T1 key_, T2 *value_) : key(key_), value(value_){};
        T1 key{};                      // reference to the key in the histogram
        T2 *value{};                   // a pointer to the histogram's object
        // these comparisions only look at the keys
        bool operator==(const item& a) const { return (this->key == a.key); }
        bool operator!=(const item& a) const { return !(*this == a); }
        bool operator<(const item& a) const {
            if (this->key < a.key) return true;
            return false;
        }
        static bool compare(const item& e1, const item& e2) { return e1 < e2; }
        virtual ~item(){};
        size_t bytes() const {
            return sizeof(*this) + value->bytes();
        } // number of bytes used by object
    };

    std::vector<item> items() const {
        std::vector<item> ret;
        /* Protect access to mymap with mutex */
        const std::lock_guard<std::mutex> lock(M);
        for ( auto &pair:mymap ){
            ret.push_back( item(pair.first, pair.second));
        }
        return ret;
    }
    void write(std::ostream &os) const {
        const std::lock_guard<std::mutex> lock(M);
        for (const auto &it : mymap) {
            os << " " << it.first << ": " << (it.second) << "\n";
        }
    }
};

#endif
