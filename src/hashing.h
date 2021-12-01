#ifndef HASHING_H
#define HASHING_H

#include <string>
#include <unordered_set>
#include "xxhash.h"

typedef XXH128_hash_t SxpHash;
typedef std::pair<const SxpHash, std::string> Trace;

inline void hash_combine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline bool operator==(const SxpHash& h1, const SxpHash& h2) {
    return XXH128_isEqual(h1, h2);
}

namespace std {
template <>
struct hash<SxpHash> {
    size_t operator()(const SxpHash& c) const {
        size_t result = 0;
        hash_combine(result, c.low64);
        hash_combine(result, c.high64);
        return result;
    }
};

template <>
struct hash<Trace> {
    size_t operator()(const Trace& c) const {
        size_t result = 0;
        hash<SxpHash> sxp_hash;
        hash<string> str_hash;

        hash_combine(result, sxp_hash(c.first));
        hash_combine(result, str_hash(c.second));
        return result;
    }
};
} // namespace std
#endif // HASHING_H

