// SocketChannel.h — copied from subprojects/ara-sdk/ARA_Library/test/SocketChannel.h
// into yabridge's source tree so it is tracked by the parent repo and available on CI.
//
// ARA::IPC::MessageChannel backed by a Unix domain socket fd.
//
// Wire frame over the fd:
//   [int32_t  messageID ]  (4 bytes, LE)
//   [uint32_t payloadLen]  (4 bytes, LE)
//   [uint8_t  payload[payloadLen]]

#pragma once

#define ARA_ENABLE_IPC 1
#include "ARAIPCConnection.h"
// SocketEncoder.h lives in the same directory in yabridge's source tree.
#include "SocketEncoder.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>

namespace SocketIPC {

namespace detail {

inline bool writeAll(int fd, const void* buf, size_t n) {
    const auto* p = static_cast<const uint8_t*>(buf);
    while (n > 0) {
        ssize_t r = ::write(fd, p, n);
        if (r <= 0) return false;
        p += r; n -= (size_t)r;
    }
    return true;
}

inline bool readAll(int fd, void* buf, size_t n) {
    auto* p = static_cast<uint8_t*>(buf);
    while (n > 0) {
        ssize_t r = ::read(fd, p, n);
        if (r <= 0) return false;
        p += r; n -= (size_t)r;
    }
    return true;
}

} // namespace detail

class SocketChannel : public ARA::IPC::MessageChannel {
public:
    explicit SocketChannel(int fd) : _fd(fd) {
        _recvThread = std::thread([this] { _receiveLoop(); });
    }

    ~SocketChannel() override {
        _stop.store(true, std::memory_order_release);
        ::shutdown(_fd, SHUT_RDWR);
        if (_recvThread.joinable()) {
            try { _recvThread.join(); }
            catch (const std::system_error& e) {
                std::fprintf(stderr, "[SocketChannel fd=%d] join error: %s\n",
                             _fd, e.what());
            }
        }
        ::close(_fd);
    }

    void sendMessage(ARA::IPC::MessageID messageID,
                     std::unique_ptr<ARA::IPC::MessageEncoder>&& encoder) override {
        auto payload = serializeEncoder(static_cast<const SocketEncoder&>(*encoder));
        uint32_t hdr[2];
        hdr[0] = (uint32_t)messageID;
        hdr[1] = (uint32_t)payload.size();
        std::lock_guard<std::mutex> lk(_sendMutex);
        detail::writeAll(_fd, hdr, 8);
        if (!payload.empty())
            detail::writeAll(_fd, payload.data(), payload.size());
    }

    bool receivesMessagesOnCurrentThread() override { return false; }
    bool waitForMessageOnCurrentThread()   override { return false; }

private:
    void _receiveLoop() {
        while (!_stop.load(std::memory_order_acquire)) {
            uint32_t hdr[2];
            if (!detail::readAll(_fd, hdr, 8)) break;
            auto messageID = static_cast<ARA::IPC::MessageID>((int32_t)hdr[0]);
            uint32_t payloadLen = hdr[1];
            std::unique_ptr<ARA::IPC::MessageDecoder> decoder;
            if (payloadLen > 0) {
                std::vector<uint8_t> payload(payloadLen);
                if (!detail::readAll(_fd, payload.data(), payloadLen)) break;
                decoder = deserializeDecoder(payload.data(), payload.size());
            }
            routeReceivedMessage(messageID, std::move(decoder));
        }
    }

    int        _fd;
    std::mutex _sendMutex;
    std::thread _recvThread;
    std::atomic<bool> _stop { false };
};

} // namespace SocketIPC
