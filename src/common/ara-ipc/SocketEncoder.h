// SocketEncoder.h — copied from subprojects/ara-sdk/ARA_Library/test/SocketEncoder.h
// into yabridge's source tree so it is tracked by the parent repo and available on CI.
//
// Minimal MessageEncoder / MessageDecoder backed by a std::map<int32_t, Value>.
// Wire format (produced by SocketEncoder::serialize):
//
//   [uint32_t entry_count]
//   for each entry:
//     [int32_t  key ]
//     [uint8_t  tag ]   0=i32, 1=i64, 2=sz, 3=f32, 4=f64, 5=str, 6=bytes, 7=sub
//     [value bytes  ]
//       i32  : 4 bytes LE
//       i64  : 8 bytes LE
//       sz   : 8 bytes LE  (always 64-bit on the wire, simplifies cross-arch)
//       f32  : 4 bytes
//       f64  : 8 bytes
//       str  : [uint32_t len] [len bytes, no NUL]
//       bytes: [uint32_t len] [len bytes]
//       sub  : [uint32_t len] [len bytes of nested serialised message]
//
// ARA IPC always runs between two ends of the same socketpair on Linux (same
// machine, same arch, same endianness), so we write values in native byte
// order — the Connection is told receiverEndianessMatches = true.

#pragma once

#define ARA_ENABLE_IPC 1
#include "ARA_Library/IPC/ARAIPCMessage.h"

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace SocketIPC {

// ---------------------------------------------------------------------------
// Value variant stored inside the encoder
// ---------------------------------------------------------------------------

struct SubMsg; // forward

using Value = std::variant<
    int32_t,
    int64_t,
    uint64_t,         // size_t  (stored as u64 on wire)
    float,
    double,
    std::string,      // string or bytes — distinguished by tag
    std::vector<uint8_t>,
    std::shared_ptr<SubMsg>
>;

// Tags must match the wire format described above.
enum Tag : uint8_t { T_I32=0, T_I64=1, T_SZ=2, T_F32=3, T_F64=4,
                     T_STR=5, T_BYTES=6, T_SUB=7 };

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

class SocketEncoder;
class SocketDecoder;

std::vector<uint8_t> serializeEncoder(const SocketEncoder& enc);
std::unique_ptr<SocketDecoder> deserializeDecoder(const uint8_t* data, size_t len);

// ---------------------------------------------------------------------------
// SubMsg wrapper so we can put an encoder into a Value
// ---------------------------------------------------------------------------

struct SubMsg {
    std::shared_ptr<SocketEncoder> enc;
};

// ---------------------------------------------------------------------------
// SocketEncoder
// ---------------------------------------------------------------------------

class SocketEncoder : public ARA::IPC::MessageEncoder {
public:
    SocketEncoder() = default;

    void appendInt32(ARA::IPC::MessageArgumentKey k, int32_t  v) override { _store(k, Value{v},           T_I32); }
    void appendInt64(ARA::IPC::MessageArgumentKey k, int64_t  v) override { _store(k, Value{v},           T_I64); }
    void appendSize (ARA::IPC::MessageArgumentKey k, size_t   v) override { _store(k, Value{(uint64_t)v}, T_SZ);  }
    void appendFloat(ARA::IPC::MessageArgumentKey k, float    v) override { _store(k, Value{v},           T_F32); }
    void appendDouble(ARA::IPC::MessageArgumentKey k, double  v) override { _store(k, Value{v},           T_F64); }

    void appendString(ARA::IPC::MessageArgumentKey k, const char* v) override {
        _store(k, Value{std::string{v ? v : ""}}, T_STR);
    }

    void appendBytes(ARA::IPC::MessageArgumentKey k,
                     const uint8_t* data, size_t size, bool /*copy*/) override {
        std::vector<uint8_t> buf(data, data + size);
        _store(k, Value{std::move(buf)}, T_BYTES);
    }

    std::unique_ptr<ARA::IPC::MessageEncoder> appendSubMessage(ARA::IPC::MessageArgumentKey k) override {
        auto subEnc = std::make_unique<SocketEncoder>();
        SocketEncoder* rawPtr = subEnc.get();

        auto holder = std::make_shared<SubMsg>();
        holder->enc = std::shared_ptr<SocketEncoder>(std::move(subEnc));
        _store(k, Value{holder}, T_SUB);

        struct NonOwningEncoder : public ARA::IPC::MessageEncoder {
            SocketEncoder* _target;
            explicit NonOwningEncoder(SocketEncoder* t) : _target(t) {}
            void appendInt32 (ARA::IPC::MessageArgumentKey k, int32_t  v) override { _target->appendInt32 (k, v); }
            void appendInt64 (ARA::IPC::MessageArgumentKey k, int64_t  v) override { _target->appendInt64 (k, v); }
            void appendSize  (ARA::IPC::MessageArgumentKey k, size_t   v) override { _target->appendSize  (k, v); }
            void appendFloat (ARA::IPC::MessageArgumentKey k, float    v) override { _target->appendFloat (k, v); }
            void appendDouble(ARA::IPC::MessageArgumentKey k, double   v) override { _target->appendDouble(k, v); }
            void appendString(ARA::IPC::MessageArgumentKey k, const char* v) override { _target->appendString(k, v); }
            void appendBytes (ARA::IPC::MessageArgumentKey k, const uint8_t* d, size_t n, bool c) override { _target->appendBytes(k, d, n, c); }
            std::unique_ptr<ARA::IPC::MessageEncoder> appendSubMessage(ARA::IPC::MessageArgumentKey k) override { return _target->appendSubMessage(k); }
        };
        return std::make_unique<NonOwningEncoder>(rawPtr);
    }

    struct Entry {
        ARA::IPC::MessageArgumentKey key;
        Tag  tag;
        Value val;
    };
    const std::vector<Entry>& entries() const { return _entries; }

private:
    void _store(ARA::IPC::MessageArgumentKey k, Value v, Tag t) {
        for (auto& e : _entries)
            if (e.key == k) { e.val = std::move(v); e.tag = t; return; }
        _entries.push_back({k, t, std::move(v)});
    }

    std::vector<Entry> _entries;
};

// ---------------------------------------------------------------------------
// SocketDecoder
// ---------------------------------------------------------------------------

class SocketDecoder : public ARA::IPC::MessageDecoder {
public:
    struct Entry {
        ARA::IPC::MessageArgumentKey key;
        Tag  tag;
        int32_t              i32{};
        int64_t              i64{};
        uint64_t             sz{};
        float                f32{};
        double               f64{};
        std::string          str;
        std::vector<uint8_t> bytes;
        std::vector<uint8_t> subBytes;
    };

    explicit SocketDecoder(std::vector<Entry> entries)
        : _entries(std::move(entries)) {}

    bool readInt32 (ARA::IPC::MessageArgumentKey k, int32_t*  v) const override {
        auto* e = _find(k, T_I32); if (!e) { *v=0; return false; }
        *v = e->i32; return true;
    }
    bool readInt64 (ARA::IPC::MessageArgumentKey k, int64_t*  v) const override {
        auto* e = _find(k, T_I64); if (!e) { *v=0; return false; }
        *v = e->i64; return true;
    }
    bool readSize  (ARA::IPC::MessageArgumentKey k, size_t*   v) const override {
        auto* e = _find(k, T_SZ); if (!e) { *v=0; return false; }
        *v = (size_t)e->sz; return true;
    }
    bool readFloat (ARA::IPC::MessageArgumentKey k, float*    v) const override {
        auto* e = _find(k, T_F32); if (!e) { *v=0; return false; }
        *v = e->f32; return true;
    }
    bool readDouble(ARA::IPC::MessageArgumentKey k, double*   v) const override {
        auto* e = _find(k, T_F64); if (!e) { *v=0; return false; }
        *v = e->f64; return true;
    }
    bool readString(ARA::IPC::MessageArgumentKey k, const char** v) const override {
        auto* e = _find(k, T_STR); if (!e) { *v=nullptr; return false; }
        *v = e->str.c_str(); return true;
    }
    bool readBytesSize(ARA::IPC::MessageArgumentKey k, size_t* sz) const override {
        auto* e = _find(k, T_BYTES); if (!e) { *sz=0; return false; }
        *sz = e->bytes.size(); return true;
    }
    void readBytes(ARA::IPC::MessageArgumentKey k, uint8_t* out) const override {
        auto* e = _find(k, T_BYTES); if (!e) return;
        std::memcpy(out, e->bytes.data(), e->bytes.size());
    }
    std::unique_ptr<ARA::IPC::MessageDecoder> readSubMessage(ARA::IPC::MessageArgumentKey k) const override {
        auto* e = _find(k, T_SUB); if (!e) return nullptr;
        return deserializeDecoder(e->subBytes.data(), e->subBytes.size());
    }
    bool hasDataForKey(ARA::IPC::MessageArgumentKey k) const override {
        for (auto& e : _entries) if (e.key == k) return true;
        return false;
    }

private:
    const Entry* _find(ARA::IPC::MessageArgumentKey k, Tag t) const {
        for (auto& e : _entries) if (e.key == k && e.tag == t) return &e;
        return nullptr;
    }
    std::vector<Entry> _entries;
};

// ---------------------------------------------------------------------------
// Serialization helpers
// ---------------------------------------------------------------------------

namespace detail {

inline void appendU8 (std::vector<uint8_t>& buf, uint8_t  v) { buf.push_back(v); }
inline void appendU32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back((v)      & 0xFF); buf.push_back((v >> 8) & 0xFF);
    buf.push_back((v >>16) & 0xFF); buf.push_back((v >>24) & 0xFF);
}
inline void appendI32(std::vector<uint8_t>& buf, int32_t  v) { appendU32(buf, (uint32_t)v); }
inline void appendU64(std::vector<uint8_t>& buf, uint64_t v) {
    appendU32(buf, (uint32_t)(v & 0xFFFFFFFF));
    appendU32(buf, (uint32_t)(v >> 32));
}
inline void appendI64(std::vector<uint8_t>& buf, int64_t  v) { appendU64(buf, (uint64_t)v); }
inline void appendF32(std::vector<uint8_t>& buf, float    v) { uint32_t tmp; std::memcpy(&tmp,&v,4); appendU32(buf,tmp); }
inline void appendF64(std::vector<uint8_t>& buf, double   v) { uint64_t tmp; std::memcpy(&tmp,&v,8); appendU64(buf,tmp); }
inline void appendBytes(std::vector<uint8_t>& buf, const uint8_t* d, size_t n) {
    appendU32(buf, (uint32_t)n);
    buf.insert(buf.end(), d, d+n);
}

inline uint8_t  readU8 (const uint8_t*& p) { return *p++; }
inline uint32_t readU32(const uint8_t*& p) {
    uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
                 ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
    p += 4; return v;
}
inline int32_t  readI32(const uint8_t*& p) { return (int32_t)readU32(p); }
inline uint64_t readU64(const uint8_t*& p) {
    uint64_t lo = readU32(p); uint64_t hi = readU32(p);
    return lo | (hi << 32);
}
inline int64_t  readI64(const uint8_t*& p) { return (int64_t)readU64(p); }
inline float    readF32(const uint8_t*& p) { uint32_t t=readU32(p); float  v; std::memcpy(&v,&t,4); return v; }
inline double   readF64(const uint8_t*& p) { uint64_t t=readU64(p); double v; std::memcpy(&v,&t,8); return v; }

} // namespace detail

inline void serializeEncoderInto(const SocketEncoder& enc, std::vector<uint8_t>& buf);

inline std::vector<uint8_t> serializeEncoder(const SocketEncoder& enc) {
    std::vector<uint8_t> buf;
    serializeEncoderInto(enc, buf);
    return buf;
}

inline void serializeEncoderInto(const SocketEncoder& enc, std::vector<uint8_t>& buf) {
    using namespace detail;
    const auto& entries = enc.entries();
    appendU32(buf, (uint32_t)entries.size());
    for (const auto& e : entries) {
        appendI32(buf, e.key);
        appendU8 (buf, (uint8_t)e.tag);
        switch (e.tag) {
        case T_I32:   appendI32(buf, std::get<int32_t >(e.val)); break;
        case T_I64:   appendI64(buf, std::get<int64_t >(e.val)); break;
        case T_SZ:    appendU64(buf, std::get<uint64_t>(e.val)); break;
        case T_F32:   appendF32(buf, std::get<float   >(e.val)); break;
        case T_F64:   appendF64(buf, std::get<double  >(e.val)); break;
        case T_STR: {
            const auto& s = std::get<std::string>(e.val);
            appendBytes(buf, (const uint8_t*)s.data(), s.size());
            break;
        }
        case T_BYTES: {
            const auto& b = std::get<std::vector<uint8_t>>(e.val);
            appendBytes(buf, b.data(), b.size());
            break;
        }
        case T_SUB: {
            auto subHolder = std::get<std::shared_ptr<SubMsg>>(e.val);
            std::vector<uint8_t> subBuf;
            serializeEncoderInto(*subHolder->enc, subBuf);
            appendBytes(buf, subBuf.data(), subBuf.size());
            break;
        }
        }
    }
}

inline std::unique_ptr<SocketDecoder> deserializeDecoder(const uint8_t* data, size_t len) {
    using namespace detail;
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    auto safeRead = [&](size_t n) -> bool { return (size_t)(end - p) >= n; };
    if (!safeRead(4)) return nullptr;
    uint32_t count = readU32(p);
    std::vector<SocketDecoder::Entry> entries;
    entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (!safeRead(5)) return nullptr;
        SocketDecoder::Entry e;
        e.key = readI32(p);
        e.tag = (Tag)readU8(p);
        switch (e.tag) {
        case T_I32: if (!safeRead(4)) return nullptr; e.i32 = readI32(p); break;
        case T_I64: if (!safeRead(8)) return nullptr; e.i64 = readI64(p); break;
        case T_SZ:  if (!safeRead(8)) return nullptr; e.sz  = readU64(p); break;
        case T_F32: if (!safeRead(4)) return nullptr; e.f32 = readF32(p); break;
        case T_F64: if (!safeRead(8)) return nullptr; e.f64 = readF64(p); break;
        case T_STR: {
            if (!safeRead(4)) return nullptr;
            uint32_t n = readU32(p); if (!safeRead(n)) return nullptr;
            e.str.assign((const char*)p, n); p += n; break;
        }
        case T_BYTES: {
            if (!safeRead(4)) return nullptr;
            uint32_t n = readU32(p); if (!safeRead(n)) return nullptr;
            e.bytes.assign(p, p+n); p += n; break;
        }
        case T_SUB: {
            if (!safeRead(4)) return nullptr;
            uint32_t n = readU32(p); if (!safeRead(n)) return nullptr;
            e.subBytes.assign(p, p+n); p += n; break;
        }
        }
        entries.push_back(std::move(e));
    }
    return std::make_unique<SocketDecoder>(std::move(entries));
}

inline std::unique_ptr<ARA::IPC::MessageEncoder> makeSocketEncoder() {
    return std::make_unique<SocketEncoder>();
}

} // namespace SocketIPC
