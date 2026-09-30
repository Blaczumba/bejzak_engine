#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <span>
#include <system_error>
#include <tuple>
#include <variant>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

namespace common::networking {

#ifdef _WIN32
using socket_t = SOCKET;
constexpr socket_t INVALID_SOCKET_VAL = INVALID_SOCKET;
#else
using socket_t = int;
constexpr socket_t INVALID_SOCKET_VAL = -1;
#endif

template <typename T>
using SocketResult = std::expected<T, std::error_code>;

enum class ShutdownMode : uint8_t {
  SEND = 0,
  RECEIVE,
  SEND_RECEIVE
};

class Endpoint {
public:
  using NativeStorage = std::variant<sockaddr_in, sockaddr_in6>;

  Endpoint() = default;

  static SocketResult<Endpoint> createIpv4(const std::string& ip, uint16_t port) noexcept;

  static SocketResult<Endpoint> createIpv6(const std::string& ip, uint16_t port) noexcept;

  static Endpoint fromNative(const sockaddr* addr, socklen_t len) noexcept;

  [[nodiscard]] const sockaddr* nativeHandle() const noexcept;

  [[nodiscard]] socklen_t nativeSize() const noexcept;

  [[nodiscard]] int family() const noexcept;

private:
  NativeStorage _storage{sockaddr_in{}};
};

class Socket {
public:
  Socket(int domain, int type, int protocol = 0);

  Socket(Socket&& other) noexcept;

  Socket& operator=(Socket&& other) noexcept;

  virtual ~Socket();

  Socket(const Socket&) = delete;

  Socket& operator=(const Socket&) = delete;

  SocketResult<void> setReceiveTimeout(std::chrono::milliseconds timeout) noexcept;

  SocketResult<void> setSendTimeout(std::chrono::milliseconds timeout) noexcept;

  SocketResult<void> close() noexcept;

  [[nodiscard]] bool isValid() const noexcept;

  [[nodiscard]] socket_t nativeHandle() const noexcept {
    return _handle;
  }
  [[nodiscard]] int domain() const noexcept {
    return _domain;
  }

protected:
  explicit Socket(socket_t handle, int domain = AF_UNSPEC) noexcept;

  socket_t _handle = INVALID_SOCKET_VAL;
  int _domain = AF_UNSPEC;

private:
#ifdef _WIN32
  struct WSAInit {
    WSAInit();

    ~WSAInit();
  };
  static inline WSAInit s_wsaInit;
#endif
};

class TcpSocket final : public Socket {
  explicit TcpSocket(socket_t handle, int domain) noexcept;

  explicit TcpSocket(int domain);

public:
  TcpSocket() noexcept = default;

  static TcpSocket createIpv4Socket();

  static TcpSocket createIpv6Socket();

  ~TcpSocket() override = default;

  TcpSocket(TcpSocket&& other) noexcept = default;

  TcpSocket& operator=(TcpSocket&& other) noexcept;

  SocketResult<void> bind(const std::string& ip, uint16_t port) noexcept;

  SocketResult<void> connect(const Endpoint& endpoint) noexcept;

  SocketResult<void> listen(int backlog = SOMAXCONN) noexcept;

  SocketResult<std::tuple<TcpSocket, Endpoint>> accept() noexcept;

  SocketResult<int64_t> send(std::span<const std::byte> buffer, int flags = 0) noexcept;

  SocketResult<int64_t> recv(std::span<std::byte> buffer, int flags = 0) noexcept;

  SocketResult<void> shutdown(ShutdownMode mode) noexcept;
};

class UdpSocket final : public Socket {
  explicit UdpSocket(int domain);

public:
  UdpSocket() noexcept = default;

  static UdpSocket createIpv4Socket();

  static UdpSocket createIpv6Socket();

  ~UdpSocket() override = default;

  UdpSocket(UdpSocket&& other) noexcept = default;

  UdpSocket& operator=(UdpSocket&& other) noexcept;

  SocketResult<void> setBroadcastEnablement(bool enable) noexcept;

  SocketResult<void> bind(const std::string& ip, uint16_t port) noexcept;

  SocketResult<int64_t> sendTo(
      std::span<const std::byte> buffer, const Endpoint& destination, int flags = 0) noexcept;

  SocketResult<std::tuple<int64_t, Endpoint>> recvFrom(
      std::span<std::byte> buffer, int flags = 0) noexcept;
};

}  // namespace common::networking
