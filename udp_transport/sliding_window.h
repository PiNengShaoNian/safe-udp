#pragma once

#include <vector>

#include "buffer.h"

namespace safe_udp {
class SlidingWindow {
 public:
  SlidingWindow();
  ~SlidingWindow();

  int AddToBuffer(const SlideWinBuffer &buffer);

  std::vector<SlideWinBuffer> sliding_window_buffers_;
  int last_packet_sent_;
  int last_acked_packet_;
  // 成功发送且已经确认的数据包中最小的序列号
  int send_base_;
  // 已经收到了几次重复的ack数据包
  int dup_ack_;
};
}  // namespace safe_udp