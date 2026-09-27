#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>

#ifdef _WIN32
  #include <winsock2.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

namespace common::networking {

enum class ShutdownMode : uint8_t {
  SEND = 0,
  RECEIVE,
  SEND_RECEIVE
};

template <typename T>
using SocketResult = std::expected<T, std::error_code>;

#ifdef _WIN32
using socket_t = SOCKET;
constexpr socket_t INVALID_SOCKET_VAL = INVALID_SOCKET;
#else
using socket_t = int;
constexpr socket_t INVALID_SOCKET_VAL = -1;
#endif

class Socket {
public:
  Socket(int domain, int type, int protocol = 0);

  Socket(Socket&& other) noexcept;

  Socket& operator=(Socket&& other) noexcept;

  ~Socket();

  SocketResult<void> close() noexcept;

  bool isValid() const noexcept;

protected:
  explicit Socket(socket_t handle);

  socket_t _handle = INVALID_SOCKET_VAL;
};

class IpSocket : public Socket {
protected:
  IpSocket(int type, int protocol);

  explicit IpSocket(socket_t handle);

public:
  ~IpSocket() = default;

  IpSocket(IpSocket&& other) noexcept = default;

  IpSocket& operator=(IpSocket&& other) noexcept = default;

  SocketResult<void> bind(const std::string& address, uint16_t port);

  SocketResult<void> bind(const char* const address, uint16_t port);

  SocketResult<void> connect(const std::string& address, uint16_t port);

  SocketResult<void> connect(const char* const address, uint16_t port);

  SocketResult<int64_t> send(std::span<const char> buffer, int flags = 0);

  SocketResult<int64_t> recv(std::span<char> buffer, int flags = 0);

  SocketResult<void> setReceiveTimeout(std::chrono::milliseconds timeout);

  SocketResult<void> setSendTimeout(std::chrono::milliseconds timeout);
};

class TcpSocket final : public IpSocket {
  explicit TcpSocket(socket_t socket);

public:
  TcpSocket();

  ~TcpSocket() = default;

  TcpSocket(TcpSocket&& other) noexcept = default;

  TcpSocket& operator=(TcpSocket&& other) noexcept = default;

  SocketResult<void> listen(int backlog = SOMAXCONN);

  SocketResult<std::tuple<TcpSocket, sockaddr_in>> accept();

  SocketResult<void> shutdown(ShutdownMode mode);
};

class UdpSocket final : public IpSocket {
public:
  UdpSocket();

  ~UdpSocket() = default;

  UdpSocket(UdpSocket&& other) noexcept = default;

  UdpSocket& operator=(UdpSocket&& other) noexcept = default;

  SocketResult<int64_t> sendTo(
      std::span<const char> buffer, const std::string& address, uint16_t port, int flags = 0);

  SocketResult<int64_t> sendTo(
      std::span<const char> buffer, const char* const address, uint16_t port, int flags = 0);

  SocketResult<std::tuple<int64_t, sockaddr_in>> recvFrom(std::span<char> buffer, int flags = 0);
};

}  // namespace common::networking
