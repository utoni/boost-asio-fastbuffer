#include "fastbuffer.hpp"

#include <iostream>

#define LEN(x) (sizeof(x) / sizeof(x[0]))
#define STRLEN(x) (LEN(x) - 1)
#define TO_STR(x) ("" #x)
#define TEST_ASSERT_LINE(file, line, x)                                        \
  if ((x) == 0)                                                                \
    throw std::runtime_error((boost::format("Test failed at %1%:%2%: %3%") %   \
                              std::string(file) % line %                       \
                              std::string(TO_STR(x)))                          \
                                 .str());
#define TEST_ASSERT(x) TEST_ASSERT_LINE(__FILE__, __LINE__, (x))

using std::cout;

auto selftest() {
  BufferBase base(16);

  TEST_ASSERT(base.capacity() == 16);
  TEST_ASSERT(base.unused() == 16);
  base += 3;
  base += {0xFF, 0xFF, 0xFF};
  TEST_ASSERT(base.size() == 6);
  TEST_ASSERT(base.unused() == 10);
  static constexpr uint8_t test_buffer[] = "\xFF\xFF\xFF";
  TEST_ASSERT(memcmp(test_buffer, base(3), STRLEN(test_buffer)) == 0);
  base -= 2;
  TEST_ASSERT(base.unconsumed() == 4);
  TEST_ASSERT(base.size() - 3 == STRLEN(test_buffer));
  TEST_ASSERT(base.unused() == 10);
  base -= 2;
  TEST_ASSERT(base.unconsumed() == 2);
  TEST_ASSERT(base.size() - 3 == STRLEN(test_buffer));
  base -= 2;
  TEST_ASSERT(base.unconsumed() == 0);
  TEST_ASSERT(base.size() == 0);
  TEST_ASSERT(base.unused() == 16);
  base += 4;
  TEST_ASSERT(base.unconsumed() == 4);
  TEST_ASSERT(base.size() == 4);
  TEST_ASSERT(base.unused() == 12);
  TEST_ASSERT(base.size() + base.unused() == base.capacity());
  TEST_ASSERT(base.getHealth() == 0.25f);
  TEST_ASSERT(base.getConsumeHealth() == 0.25f);
  base -= 3;
  TEST_ASSERT(base.getHealth() == 0.25f);
  TEST_ASSERT(base.getConsumeHealth() == 0.0625f);
  auto moved_base = BufferBase(std::move(base));
  TEST_ASSERT(base.capacity() == base.size() && base.unused() == base.size() &&
              base.unconsumed() == base.size() && base.size() == 0);

  ContiguousPacketQueue queue(8, 64);
  queue += 4;
  queue += 8;
  queue += {0xFF, 0xFF, 0xFF};
  queue += 16;
  auto buf = queue.GetBuffer();
  TEST_ASSERT(buf->size() == 31);
  TEST_ASSERT(buf->getHealth() == buf->getConsumeHealth() &&
              buf->getHealth() == 0.484375f);
  TEST_ASSERT(queue.size() == 4);
  TEST_ASSERT(queue.capacity() == 8);
  TEST_ASSERT(queue.unused() == 4);
  TEST_ASSERT(queue.unconsumed() == 4);
  queue += 3;
  TEST_ASSERT(queue.unused() == 3);
  TEST_ASSERT(queue.unconsumed() == 5);
  TEST_ASSERT(queue.size() + queue.unused() == queue.capacity());
  --queue;
  TEST_ASSERT(queue.size() == 5);
  TEST_ASSERT(queue.unconsumed() == 4);
  TEST_ASSERT(queue.size() + queue.unused() == queue.capacity());
  --queue;
  --queue;
  --queue;
  --queue;
  TEST_ASSERT(queue.size() == 0);
  TEST_ASSERT(queue.unconsumed() == 0);
  TEST_ASSERT(queue.size() + queue.unused() == queue.capacity());
  queue += {0xDE, 0xAD, 0xC0, 0xDE};
  auto moved_queue = ContiguousPacketQueue(std::move(queue));
  TEST_ASSERT(queue.capacity() == queue.size() &&
              queue.unused() == queue.size() &&
              queue.unconsumed() == queue.size() && queue.size() == 0);
}

int main(void) {
  cout << "Selftest..\n";
  selftest();

  return 0;
}
