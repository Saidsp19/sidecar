#ifndef SIDECAR_UTILS_ORDEREDINDEX_H // -*- C++ -*-
#define SIDECAR_UTILS_ORDEREDINDEX_H

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

#include "Utils/Utils.h" // for operator >>(std::string, TrackID)

namespace Utils {

/** Container of sortable keys that maintains index associations with the keys. Useful when combined with a
    std::vector of values. The keys are stored in this container, and values in the std::vector. To find a keyed
    value, invoke the find() method to obtain an iterator to an OrderedIndex::Entry object that contains the key
    and an index. The index value can then be used to obtain a value from a std::vector.

    \code
    OrderedIndex<int> keys;
    std::vector<std::string> values;
    keys.push_back(10); values.push_back("ten");
    keys.push_back(1); values.push_back("one");
    keys.push_back(5); values.push_back("five");

    OrderedIndex<int>::const_iterator it = keys.find(5);
    const std::string& value = values[it->index];
    \endcode
*/
template <typename T>
class OrderedIndex {
public:
    struct Entry {
        T key;
        unsigned long index;
        Entry() : key(), index() {}
        Entry(const T& k) : key(k), index() {}
        Entry(const T& k, unsigned long i) : key(k), index(i) {}

        bool operator<(const Entry& rhs) const { return key < rhs.key; }
    };

    using value_type = T;
    using reference = T&;
    using const_reference = const T&;

    using Container = std::vector<Entry>;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using const_reverse_iterator = typename Container::const_reverse_iterator;

    OrderedIndex() : _entries(), _ordered(true) {}

    OrderedIndex(const std::vector<T>& init) : _entries(), _ordered(true)
    {
        std::copy(init.begin(), init.end(), std::back_inserter(this));
    }

    bool empty() const { return _entries.empty(); }

    size_type size() const { return _entries.size(); }

    void clear()
    {
        _entries.clear();
        _ordered = true;
    }

    void setKey(int index, const T& key)
    {
        _entries[index].key = key;
        _ordered = false;
    }

    void push_back(const T& key)
    {
        _ordered = _ordered && (_entries.empty() || _entries.back().key < key || _entries.back().key == key);
        _entries.push_back(Entry(key, _entries.size()));
    }

    void sort()
    {
        std::sort(_entries.begin(), _entries.end());
        _ordered = true;
    }

    const_iterator find(const T& k) const
    {
        sortCheck();
        auto end = _entries.end();
        auto pos = std::lower_bound(_entries.begin(), _entries.end(), Entry(k));
        return (pos != end && pos->key == k) ? pos : end;
    }

    bool contains(const T& k) const { return find(k) != end(); }

    const_iterator begin() const
    {
        sortCheck();
        return _entries.begin();
    }
    const_iterator end() const
    {
        sortCheck();
        return _entries.end();
    }

    const_reverse_iterator rbegin() const
    {
        sortCheck();
        return _entries.rbegin();
    }
    const_reverse_iterator rend() const
    {
        sortCheck();
        return _entries.rend();
    }

    void reserve(size_t size) { _entries.reserve(size); }

    void erase(const_iterator pos)
    {
        if (pos == _entries.end()) return;
        auto index = pos->index;
        _entries.erase(_entries.begin() + (pos - _entries.begin()));
        std::for_each(_entries.begin(), _entries.end(), [index](auto& e) {
            if (e.index > index) --e.index;
        });
    }

private:
    void sortCheck() const
    {
        if (!_ordered) const_cast<OrderedIndex*>(this)->sort();
    }

    Container _entries;
    bool _ordered;
};

} // namespace Utils

/** \file
 */

#endif
