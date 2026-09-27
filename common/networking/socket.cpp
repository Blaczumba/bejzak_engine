#include "socket.h"

#include <cerrno>
#include <utility>

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

std::unexpected<std::error_code> addressFamilyMismatchError() noexcept {
  return std::unexpected(std::make_error_code(std::errc::address_family_not_supported));
}

SocketResult<void> setOption(
    socket_t handle, int level, int name, const void* value, size_t size) noexcept {
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

SocketResult<void> setTimeoutOption(
    socket_t handle, int name, std::chrono::milliseconds timeout) noexcept {
#ifdef _WIN32
  const DWORD value = static_cast<DWORD>(timeout.count());
#else
  timeval value{.tv_sec = static_cast<time_t>(timeout.count() / 1000),
                .tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000)};
#endif
  return setOption(handle, SOL_SOCKET, name, &value, sizeof(value));
}

}  // namespace

SocketResult<Endpoint> Endpoint::createIpv4(const std::string& address, uint16_t port) noexcept {
  Endpoint ep;
  sockaddr_in addr{.sin_family = AF_INET, .sin_port = htons(port)};
  if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
    return invalidAddressError();
  }
  ep._storage = addr;
  return ep;
}

SocketResult<Endpoint> Endpoint::createIpv6(const std::string& address, uint16_t port) noexcept {
  Endpoint ep;
  sockaddr_in6 addr{.sin6_family = AF_INET6, .sin6_port = htons(port)};
  if (inet_pton(AF_INET6, address.c_str(), &addr.sin6_addr) != 1) {
    return invalidAddressError();
  }
  ep._storage = addr;
  return ep;
}

Endpoint Endpoint::fromNative(const sockaddr* addr, socklen_t len) noexcept {
  Endpoint ep;
  if (!addr) {
    return ep;
  }

  if (addr->sa_family == AF_INET && len >= sizeof(sockaddr_in)) {
    sockaddr_in in{};
    std::memcpy(&in, addr, sizeof(sockaddr_in));
    ep._storage = in;
  } else if (addr->sa_family == AF_INET6 && len >= sizeof(sockaddr_in6)) {
    sockaddr_in6 in6{};
    std::memcpy(&in6, addr, sizeof(sockaddr_in6));
    ep._storage = in6;
  }
  return ep;
}

const sockaddr* Endpoint::nativeHandle() const noexcept {
  return std::visit(
      [](const auto& addr) -> const sockaddr* {
        return reinterpret_cast<const sockaddr*>(&addr);
      },
      _storage);
}

socklen_t Endpoint::nativeSize() const noexcept {
  return std::visit(
      [](const auto& addr) -> socklen_t {
        return sizeof(addr);
      },
      _storage);
}

struct FamilyVisitor {
  int operator()(const sockaddr_in& addr) const noexcept {
    return addr.sin_family;
  }

  int operator()(const sockaddr_in6& addr) const noexcept {
    return addr.sin6_family;
  }
};

int Endpoint::family() const noexcept {
  return std::visit(FamilyVisitor{}, _storage);
}

#ifdef _WIN32
Socket::WSAInit::WSAInit() {
  WSADATA wsa{};
  WSAStartup(MAKEWORD(2, 2), &wsa);
}

Socket::WSAInit::~WSAInit() {
  WSACleanup();
}
#endif

Socket::Socket(int domain, int type, int protocol)
  : _handle(::socket(domain, type, protocol)), _domain(domain) {}

Socket::Socket(socket_t handle, int domain) noexcept : _handle(handle), _domain(domain) {}

Socket::Socket(Socket&& other) noexcept
  : _handle(std::exchange(other._handle, INVALID_SOCKET_VAL)),
    _domain(std::exchange(other._domain, AF_UNSPEC)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
  if (this != &other) {
    (void)close();
    _handle = std::exchange(other._handle, INVALID_SOCKET_VAL);
    _domain = std::exchange(other._domain, AF_UNSPEC);
  }
  return *this;
}

Socket::~Socket() {
  (void)close();
}

SocketResult<void> Socket::close() noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return {};
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

SocketResult<void> Socket::setReceiveTimeout(std::chrono::milliseconds timeout) noexcept {
  return setTimeoutOption(_handle, SO_RCVTIMEO, timeout);
}

SocketResult<void> Socket::setSendTimeout(std::chrono::milliseconds timeout) noexcept {
  return setTimeoutOption(_handle, SO_SNDTIMEO, timeout);
}

TcpSocket::TcpSocket(int domain) : Socket(domain, SOCK_STREAM, IPPROTO_TCP) {}

TcpSocket::TcpSocket(socket_t socket, int domain) noexcept : Socket(socket, domain) {}

TcpSocket TcpSocket::createIpv4Socket() {
  return TcpSocket(AF_INET);
}
TcpSocket TcpSocket::createIpv6Socket() {
  return TcpSocket(AF_INET6);
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
  if (this != &other) {
    Socket::operator=(std::move(other));
  }
  return *this;
}

SocketResult<void> TcpSocket::bind(const std::string& ip, uint16_t port) noexcept {
  SocketResult<Endpoint> endpoint =
      _domain == AF_INET ? Endpoint::createIpv4(ip, port) : Endpoint::createIpv6(ip, port);
  if (!endpoint) {
    return std::unexpected(endpoint.error());
  }
  if (::bind(_handle, endpoint->nativeHandle(), endpoint->nativeSize()) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<void> TcpSocket::connect(const Endpoint& endpoint) noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  if (_domain != AF_UNSPEC && endpoint.family() != _domain) {
    return addressFamilyMismatchError();
  }
  if (::connect(_handle, endpoint.nativeHandle(), endpoint.nativeSize()) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<void> TcpSocket::listen(int backlog) noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  if (::listen(_handle, backlog) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<std::tuple<TcpSocket, Endpoint>> TcpSocket::accept() noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  sockaddr_storage clientAddr{};
  socklen_t clientSize = sizeof(clientAddr);

  socket_t client = ::accept(_handle, reinterpret_cast<sockaddr*>(&clientAddr), &clientSize);
  if (client == INVALID_SOCKET_VAL) {
    return std::unexpected(getLastError());
  }

  Endpoint clientEp = Endpoint::fromNative(reinterpret_cast<sockaddr*>(&clientAddr), clientSize);
  return std::tuple{TcpSocket(client, _domain), clientEp};
}

SocketResult<int64_t> TcpSocket::send(std::span<const std::byte> buffer, int flags) noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  const auto sent = ::send(_handle, reinterpret_cast<const char*>(buffer.data()),
                           static_cast<int>(buffer.size()), flags);
  if (sent < 0) {
    return std::unexpected(getLastError());
  }
  return static_cast<int64_t>(sent);
}

SocketResult<int64_t> TcpSocket::recv(std::span<std::byte> buffer, int flags) noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  const auto received = ::recv(
      _handle, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), flags);
  if (received < 0) {
    return std::unexpected(getLastError());
  }
  return static_cast<int64_t>(received);
}

SocketResult<void> TcpSocket::shutdown(ShutdownMode mode) noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
#ifdef _WIN32
  static constexpr int modes[] = {SD_SEND, SD_RECEIVE, SD_BOTH};
#else
  static constexpr int modes[] = {SHUT_WR, SHUT_RD, SHUT_RDWR};
#endif
  if (::shutdown(_handle, modes[static_cast<int>(mode)]) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

UdpSocket::UdpSocket(int domain) : Socket(domain, SOCK_DGRAM, IPPROTO_UDP) {}

UdpSocket UdpSocket::createIpv4Socket() {
  return UdpSocket(AF_INET);
}
UdpSocket UdpSocket::createIpv6Socket() {
  return UdpSocket(AF_INET6);
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
  if (this != &other) {
    Socket::operator=(std::move(other));
  }
  return *this;
}

SocketResult<void> UdpSocket::bind(const std::string& ip, uint16_t port) noexcept {
  SocketResult<Endpoint> endpoint =
      _domain == AF_INET ? Endpoint::createIpv4(ip, port) : Endpoint::createIpv6(ip, port);
  if (!endpoint) {
    return std::unexpected(endpoint.error());
  }
  if (::bind(_handle, endpoint->nativeHandle(), endpoint->nativeSize()) != 0) {
    return std::unexpected(getLastError());
  }
  return {};
}

SocketResult<int64_t> UdpSocket::sendTo(
    std::span<const std::byte> buffer, const Endpoint& destination, int flags) noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  if (_domain != AF_UNSPEC && destination.family() != _domain) {
    return addressFamilyMismatchError();
  }
  const auto sent = ::sendto(
      _handle, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), flags,
      destination.nativeHandle(), destination.nativeSize());
  if (sent < 0) {
    return std::unexpected(getLastError());
  }
  return static_cast<int64_t>(sent);
}

SocketResult<std::tuple<int64_t, Endpoint>> UdpSocket::recvFrom(
    std::span<std::byte> buffer, int flags) noexcept {
  if (_handle == INVALID_SOCKET_VAL) {
    return invalidSocketError();
  }
  sockaddr_storage senderAddr{};
  socklen_t senderSize = sizeof(senderAddr);

  const auto received =
      ::recvfrom(_handle, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()),
                 flags, reinterpret_cast<sockaddr*>(&senderAddr), &senderSize);
  if (received < 0) {
    return std::unexpected(getLastError());
  }

  Endpoint senderEp = Endpoint::fromNative(reinterpret_cast<sockaddr*>(&senderAddr), senderSize);
  return std::tuple{static_cast<int64_t>(received), senderEp};
}

}  // namespace common::networking
