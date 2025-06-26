#ifndef FASTBUFFER_H
#define FASTBUFFER_H 1

#include <boost/asio/buffer.hpp>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <exception>
#include <string>

class BufferException : public std::exception {
public:
  explicit BufferException(const char *what) : m_what(what) {}
  explicit BufferException(const std::string &what) : m_what(what) {}
  virtual ~BufferException() noexcept {}
  virtual const char *what() const noexcept { return m_what.c_str(); }

private:
  std::string m_what;
};

class BufferBase {
public:
  explicit BufferBase(std::size_t size)
      : m_bufferOffset{0}, m_bufferUsed{0}, m_bufferSize{size} {
    m_buffer = new std::uint8_t[size];
  }
  BufferBase(BufferBase &&moveable) {
    m_bufferOffset = std::move(moveable.m_bufferOffset);
    m_bufferUsed = std::move(moveable.m_bufferUsed);
    m_bufferSize = std::move(moveable.m_bufferSize);
    m_buffer = std::move(moveable.m_buffer);
    moveable.m_buffer = nullptr;
    moveable.m_bufferOffset = moveable.m_bufferUsed = moveable.m_bufferSize = 0;
  }
  ~BufferBase() { delete[] m_buffer; }
  void operator+=(std::size_t commit_size) {
    checkFreeSpace(commit_size);
    m_bufferUsed += commit_size;
  }
  void operator+=(const std::initializer_list<uint8_t> &to_add) {
    checkFreeSpace(to_add.size());
    std::copy(to_add.begin(), to_add.end(), &m_buffer[m_bufferUsed]);
    m_bufferUsed += to_add.size();
  }
  void operator+=(const std::string &to_add) {
    checkFreeSpace(to_add.size());
    std::copy(to_add.begin(), to_add.end(), &m_buffer[m_bufferUsed]);
    m_bufferUsed += to_add.size();
  }
  void operator-=(std::size_t consume_size) {
    const auto unconsumed_space = unconsumed();
    checkConsumableSpace(consume_size);
    if (consume_size == unconsumed_space) {
      m_bufferUsed = m_bufferOffset = 0;
      return;
    }
    m_bufferOffset += consume_size;
  }
  auto operator+() {
    return boost::asio::buffer(&m_buffer[m_bufferUsed], unused());
  }
  auto operator-() {
    return boost::asio::buffer(&m_buffer[m_bufferOffset], m_bufferUsed);
  }
  auto operator[](std::size_t index) const { return m_buffer[index]; }
  const auto *operator()(std::size_t index) const { return &m_buffer[index]; }
  auto *operator()() { return &m_buffer[m_bufferUsed]; }
  auto size() const { return m_bufferUsed; }
  auto capacity() const { return m_bufferSize; }
  std::size_t unused() const { return m_bufferSize - m_bufferUsed; }
  std::size_t unconsumed() const { return m_bufferUsed - m_bufferOffset; }
  void checkFreeSpace(std::size_t commit_size) const {
    const auto free_space = unused();
    if (commit_size > free_space)
      throw BufferException(
          (boost::format(
               "Buffer overflow: %1% bytes free, %1% bytes required") %
           free_space % commit_size)
              .str());
  }
  void checkConsumableSpace(std::size_t consume_size) const {
    const auto unconsumed_space = unconsumed();
    if (consume_size > unconsumed_space)
      throw BufferException(
          (boost::format(
               "Buffer underflow: %1% bytes used, %1% bytes consumed") %
           unconsumed_space % consume_size)
              .str());
  };
  auto getHealth() const {
    return (static_cast<float>(m_bufferUsed) /
            static_cast<float>(m_bufferSize));
  }
  auto getConsumeHealth() const {
    return (static_cast<float>(unconsumed()) /
            static_cast<float>(m_bufferSize));
  }

private:
  std::size_t m_bufferOffset;
  std::size_t m_bufferUsed;
  std::size_t m_bufferSize;
  std::uint8_t *m_buffer;
};

using ContiguousStreamBuffer = BufferBase;

class ContiguousPacketQueue : public boost::noncopyable {
public:
  struct Element {
    std::size_t size;
  };
  explicit ContiguousPacketQueue(std::size_t max_packets,
                                 std::size_t max_queue_size)
      : m_buffer(max_queue_size), m_packetsOffset{0}, m_packetsUsed{0},
        m_packetsSize{max_packets} {
    m_packets = new Element[max_packets];
  }
  ContiguousPacketQueue(ContiguousPacketQueue &&moveable)
      : m_buffer(std::move(moveable.m_buffer)) {
    m_packetsOffset = std::move(moveable.m_packetsOffset);
    m_packetsUsed = std::move(moveable.m_packetsUsed);
    m_packetsSize = std::move(moveable.m_packetsSize);
    m_packets = std::move(moveable.m_packets);
    moveable.m_packets = nullptr;
    moveable.m_packetsOffset = moveable.m_packetsUsed = moveable.m_packetsSize =
        0;
  }
  ~ContiguousPacketQueue() { delete[] m_packets; }
  void operator+=(std::size_t commit_size) {
    m_packets[m_packetsUsed++].size = commit_size;
    m_buffer += commit_size;
  }
  void operator+=(const std::initializer_list<uint8_t> &to_add) {
    m_packets[m_packetsUsed++].size = to_add.size();
    m_buffer += to_add;
  }
  void operator+=(const std::string &to_add) {
    m_packets[m_packetsUsed++].size = to_add.length();
    m_buffer += to_add;
  }
  void operator--() {
    const auto consume_size = m_packets[m_packetsOffset].size;
    m_buffer -= consume_size;
    if (++m_packetsOffset == m_packetsUsed) {
      m_packetsOffset = m_packetsUsed = 0;
      return;
    }
  }
  auto operator+() { return +m_buffer; }
  auto operator-() { return -m_buffer; }
  auto size() const { return m_packetsUsed; }
  auto capacity() const { return m_packetsSize; }
  std::size_t unused() const { return m_packetsSize - m_packetsUsed; }
  std::size_t unconsumed() const { return m_packetsUsed - m_packetsOffset; }
  const auto *GetBuffer() const { return &m_buffer; }

private:
  BufferBase m_buffer;
  std::size_t m_packetsOffset;
  std::size_t m_packetsUsed;
  std::size_t m_packetsSize;
  Element *m_packets;
};

#endif
