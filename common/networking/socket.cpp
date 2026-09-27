#include "common/networking/socket.h"

#include <cerrno>
#include <chrono>
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
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

namespace common::networking {
namespace {

std::error_code getLastError() noexcept {
#ifdef _WIN32
  return {WSAGetLastError(), std::system_category()};
#else
  return {errno, std::system_category()};
#endif
}

std::unexpected<std::error_code> invalidSocketError() noexcept {
  return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
}

std::unexpected<std::error_code> invalidAddressError() noexcept {
  return std::unexpected(std::make_error_code(std::errc::invalid_argument));
}

void initWinsock() {
#ifdef _WIN32
  static struct WSAInit {
    WSAInit() {
      WSADATA wsa;
      WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    ~WSAInit() {
      WSACleanup();
    }
  } wsa_init;
#endif
}

SocketResult<void> setOption(socket_t handle, int level, int name, const void* value, size_t size) {
  if (handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  if (::setsockopt(
          handle, level, name, static_cast<const char*>(value), static_cast<socklen_t>(size))
      != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<void> setTimeoutOption(socket_t handle, int name, std::chrono::milliseconds timeout) {
#ifdef _WIN32
  const DWORD value = static_cast<DWORD>(timeout.count());
#else
  timeval value{.tv_sec = static_cast<time_t>(timeout.count() / 1000),
                .tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000)};
#endif
  return setOption(handle, SOL_SOCKET, name, &value, sizeof(value));
}

}  // namespace

Socket::Socket(int domain, int type, int protocol) {
  initWinsock();
  _handle = ::socket(domain, type, protocol);
}

Socket::Socket(socket_t handle) : _handle(handle) {}

Socket::Socket(Socket&& other) noexcept
  : _handle(std::exchange(other._handle, INVALID_SOCKET_VAL)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
  if (this != &other) {
    close();
    _handle = std::exchange(other._handle, INVALID_SOCKET_VAL);
  }
  return *this;
}

Socket::~Socket() {
  close();
}

SocketResult<void> Socket::close() noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  const socket_t tmp = std::exchange(_handle, INVALID_SOCKET_VAL);
#ifdef _WIN32
  if (::closesocket(tmp) != 0) {
    return std::unexpected(getLastError());
  }
#else
  if (::close(tmp) != 0) {
    return std::unexpected(getLastError());
  }
#endif
  return {};
}

bool Socket::isValid() const noexcept {
  return _handle != INVALID_SOCKET_VAL;
}

TcpSocket::TcpSocket(socket_t socket) : Socket(socket) {}

TcpSocket::TcpSocket() : Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) {}

SocketResult<void> TcpSocket::bind(const std::string& address, uint16_t port) {
  return bind(address.c_str(), port);
}

SocketResult<void> TcpSocket::bind(const char* const address, uint16_t port) {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  sockaddr_in hint{.sin_family = AF_INET, .sin_port = htons(port)};
  if (inet_pton(AF_INET, address, &hint.sin_addr) != 1) {
    return invalidAddressError();
  }
  if (::bind(_handle, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<void> TcpSocket::listen(int backlog) {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  if (::listen(_handle, backlog) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<std::tuple<TcpSocket, sockaddr_in>> TcpSocket::accept() {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  sockaddr_in client_addr{};
  socklen_t client_size = sizeof(client_addr);
  TcpSocket client(::accept(_handle, reinterpret_cast<sockaddr*>(&client_addr), &client_size));
  if (!client.isValid()) {
    return std::unexpected(getLastError());
  }
  return std::tuple{std::move(client), client_addr};
}

SocketResult<void> TcpSocket::connect(const std::string& address, uint16_t port) {
  return connect(address.c_str(), port);
}

SocketResult<void> TcpSocket::connect(const char* const address, uint16_t port) {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  sockaddr_in hint{.sin_family = AF_INET, .sin_port = htons(port)};
  if (inet_pton(AF_INET, address, &hint.sin_addr) != 1) {
    return invalidAddressError();
  }
  if (::connect(_handle, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<int64_t> TcpSocket::send(std::span<const char> buffer, int flags) {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  const int64_t sent =
      static_cast<int64_t>(::send(_handle, buffer.data(), static_cast<int>(buffer.size()), flags));
  if (sent < 0) {
    return std::unexpected(getLastError());
  }
  return static_cast<int64_t>(sent);
}

SocketResult<int64_t> TcpSocket::recv(std::span<char> buffer, int flags) {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  const int64_t received =
      static_cast<int64_t>(::recv(_handle, buffer.data(), static_cast<int>(buffer.size()), flags));
  if (received < 0) {
    return std::unexpected(getLastError());
  }
  return static_cast<int64_t>(received);
}

SocketResult<void> TcpSocket::setReceiveTimeout(std::chrono::milliseconds timeout) {
  return setTimeoutOption(_handle, SO_RCVTIMEO, timeout);
}

SocketResult<void> TcpSocket::setSendTimeout(std::chrono::milliseconds timeout) {
  return setTimeoutOption(_handle, SO_SNDTIMEO, timeout);
}

SocketResult<void> TcpSocket::shutdown(ShutdownMode mode) {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
#ifdef _WIN32
  static constexpr int modes[] = {SD_RECEIVE, SD_SEND, SD_BOTH};
#else
  static constexpr int modes[] = {SHUT_RD, SHUT_WR, SHUT_RDWR};
#endif
  if (::shutdown(_handle, modes[static_cast<int>(mode)]) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

UdpSocket::UdpSocket() : Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) {}

}  // namespace common::networking
