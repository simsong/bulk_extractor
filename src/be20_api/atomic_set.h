/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/**
 * defines atomic_map and atomic_set.
 * This is a nice lightweight atomic set when not much else is needed.
 *
 * 2020-07-06 - slg - Upgraded to to C++17.
 */

#ifndef ATOMIC_SET_H
#define ATOMIC_SET_H

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <unordered_set>
#include <vector>

/*
 * note: do not use const TYPE &s for signatures; it caused deadlocks.
 */

template <class TYPE> class atomic_set {
    // Mutex M protects myset.
    // It is mutable to allow modification in const methods
    mutable std::mutex M{};
    std::set<TYPE> myset{};

public:
    atomic_set() {}
    ~atomic_set() {
        const std::lock_guard<std::mutex> lock(M);
        myset.clear();
    }
    void clear() {
        const std::lock_guard<std::mutex> lock(M);
        myset.clear();
    }
    bool contains(const TYPE& s) const {
        const std::lock_guard<std::mutex> lock(M);
        return myset.find(s) != myset.end();
    }
    void insert(const TYPE s) {
        const std::lock_guard<std::mutex> lock(M);
        myset.insert(s);
    }

    void erase(const TYPE s) {
        const std::lock_guard<std::mutex> lock(M);
        myset.erase(s);
    }

    /* Returns true if s is in the set, false if it is not.
     * After return, s is in the set.
     */
    bool check_for_presence_and_insert(const TYPE s) {
        const std::lock_guard<std::mutex> lock(M);
        if (myset.find(s) != myset.end()) return true; // in the set
        myset.insert(s);                               // otherwise insert it
        return false;                                  // and return that it wasn't
    }

    /* Returns true if s is in the set, false if it is not.
     * After return, s is not the set.
     */
    bool check_for_presence_and_erase(const TYPE s) {
        const std::lock_guard<std::mutex> lock(M);
        bool in_set = (myset.find(s) != myset.end());
        if (in_set){
            myset.erase(s);
        }
        return in_set;                                  // and return that it wasn't
    }


    /* returns the count, not the bytes */
    size_t size() const {
        const std::lock_guard<std::mutex> lock(M);
        return myset.size();
    }
    /* like python .keys() */
    std::vector<TYPE> keys() const {
        const std::lock_guard<std::mutex> lock(M);
        std::vector<TYPE> ret;
        for (auto obj: myset) {
            ret.push_back(obj);
        }
        return ret;
    }
};

#endif
