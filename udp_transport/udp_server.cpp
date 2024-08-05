#include "udp_server.h"

#include <arpa/inet.h>
#include <math.h>

namespace safe_udp {
int UdpServer::StartServer(int port) {
  int sfd;
  struct sockaddr_in server_addr;
  sfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sfd < 0) {
    LOG(ERROR) << " Failed to socket !!!";
    exit(0);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(port);

  if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    LOG(ERROR) << "binding error !!!";
    exit(0);
  }

  LOG(INFO) << "**Server Bind set to addr: " << server_addr.sin_addr.s_addr;
  LOG(INFO) << "**Server Bind set to port: " << server_addr.sin_port;
  LOG(INFO) << "**Server Bind set to family: " << server_addr.sin_family;
  LOG(INFO) << "Started successfully";
  sockfd_ = sfd;
  return sfd;
}

void UdpServer::StartFileTransfer() {
  LOG(INFO) << "Starting the file_ transfer";

  file_.seekg(0, std::ios::end);
  file_length_ = file_.tellg();
  file_.seekg(0, std::ios::beg);

  send();
}

void UdpServer::send() {
  LOG(INFO) << "Entering Send()";

  int sent_count = 1;
  int sent_count_limit = 1;

  struct timeval process_start_time;
  struct timeval process_end_time;
  gettimeofday(&process_start_time, NULL);

  if (sliding_window_->last_packet_sent_ == -1) {
    start_byte_ = 0;
  }

  while (start_byte_ <= file_length_) {
    fd_set rfds;
    struct timeval tv;
    int res;

    sent_count = 1;
    sent_count_limit = std::min(rwnd_, cwnd_);
    LOG(INFO) << "SEND START !!!!";
    LOG(INFO) << "Before the window rwnd_: " << rwnd_ << " cwnd_: " << cwnd_
              << " window used: "
              << sliding_window_->last_packet_sent_ -
                     sliding_window_->last_acked_packet_;
    while (sliding_window_->last_packet_sent_ -
                   sliding_window_->last_acked_packet_ <=
               std::min(rwnd_, cwnd_) &&
           sent_count <= sent_count_limit) {
      send_packet(start_byte_ + initial_seq_number_, start_byte_);

      if (is_slow_start_) {
        packet_statistics_->slow_start_packet_sent_count_++;
      } else if (is_cong_avd_) {
        packet_statistics_->cong_avd_packet_sent_count_++;
      }

      start_byte_ = start_byte_ + MAX_DATA_SIZE;
      if (start_byte_ > file_length_) {
        LOG(INFO) << "No more data left to be sent";
        break;
      }
      sent_count++;
    }

    LOG(INFO) << "SEND END !!!!";
  }

  gettimeofday(&process_end_time, NULL);

  int64_t total_time =
      (process_end_time.tv_sec * 1000000 + process_end_time.tv_usec) -
      (process_start_time.tv_sec * 1000000 + process_start_time.tv_usec);

  int total_packet_sent = packet_statistics_->slow_start_packet_sent_count_ +
                          packet_statistics_->cong_avd_packet_sent_count_;
  LOG(INFO) << "\n";
  LOG(INFO) << "========================================";
  LOG(INFO) << "Total Time: " << (float)total_time / pow(10, 6) << " secs";
  LOG(INFO) << "Statistics: 拥塞控制--慢启动: "
            << packet_statistics_->slow_start_packet_sent_count_
            << " 拥塞控制--拥塞避免: "
            << packet_statistics_->cong_avd_packet_sent_count_;
  LOG(INFO) << "Statistics: Slow start: "
            << ((float)packet_statistics_->slow_start_packet_sent_count_ /
                total_packet_sent) *
                   100
            << "% CongAvd: "
            << ((float)packet_statistics_->cong_avd_packet_sent_count_ /
                total_packet_sent) *
                   100
            << "%";
  LOG(INFO) << "Statistics: Retransmissions: "
            << packet_statistics_->retransmit_count_;
  LOG(INFO) << "========================================";
}

void UdpServer::send_packet(int seq_number, int start_byte) {
  bool lastPacket = false;
  int dataLength = 0;
  if (file_length_ <= start_byte + MAX_DATA_SIZE) {
    LOG(INFO) << "Last packet to be sent !!!";
    dataLength = file_length_ - start_byte;
    lastPacket = true;
  } else {
    dataLength = MAX_DATA_SIZE;
  }

  struct timeval time;

  gettimeofday(&time, NULL);

  if (sliding_window_->last_packet_sent_ != -1 &&
      start_byte <
          sliding_window_
              ->sliding_window_buffers_[sliding_window_->last_acked_packet_]
              .first_byte_) {
    for (int i = sliding_window_->last_acked_packet_ + 1;
         i < sliding_window_->last_packet_sent_; i++) {
      if (sliding_window_->sliding_window_buffers_[i].first_byte_ ==
          start_byte) {
        sliding_window_->sliding_window_buffers_[i].time_sent_ = time;
        break;
      }
    }
  } else {
    SlideWinBuffer slidingWindowBuffer;
    slidingWindowBuffer.first_byte_ = start_byte;
    slidingWindowBuffer.data_length_ = dataLength;
    slidingWindowBuffer.seq_num_ = initial_seq_number_ + start_byte;
    struct timeval time;
    gettimeofday(&time, NULL);
    slidingWindowBuffer.time_sent_ = time;
    sliding_window_->last_acked_packet_ =
        sliding_window_->AddToBuffer(slidingWindowBuffer);
  }
  read_file_and_send(lastPacket, start_byte, start_byte + dataLength);
}

void UdpServer::read_file_and_send(bool fin_flag, int start_byte,
                                   int end_byte) {
  int dataLength = end_byte - start_byte;
  if (file_length_ - start_byte < dataLength) {
    dataLength = file_length_ - start_byte;
    fin_flag = true;
  }
  char *fileData = reinterpret_cast<char *>(calloc(dataLength, sizeof(char)));
  if (!file_.is_open()) {
    LOG(ERROR) << "File open failed !!!";
    free(fileData);
    return;
  }
  file_.seekg(start_byte);
  file_.read(fileData, dataLength);

  DataSegment *data_segment = new DataSegment();
  data_segment->seq_number_ = start_byte + initial_seq_number_;
  data_segment->ack_number_ = 0;
  data_segment->ack_flag_ = false;
  data_segment->fin_flag_ = fin_flag;
  data_segment->length_ = dataLength;
  data_segment->data_ = fileData;

  send_data_segment(data_segment);
  LOG(INFO) << "Packet sent: seq number: " << data_segment->seq_number_;

  free(fileData);
  delete data_segment;
}

bool UdpServer::OpenFile(const std::string &file_name) {
  LOG(INFO) << "Opening the file" << file_name;

  file_.open(file_name.c_str(), std::ios::in);
  if (!this->file_.is_open()) {
    LOG(INFO) << "File: " << file_name << " opening failed";
    return false;
  } else {
    LOG(INFO) << "File: " << file_name << " opening success";
    return true;
  }
}

char *UdpServer::GetRequest(int server_sockfd) {
  char *buffer =
      reinterpret_cast<char *>(calloc(MAX_PACKET_SIZE, sizeof(char)));
  struct sockaddr_in client_addr;
  socklen_t addr_size;
  memset(buffer, 0, MAX_PACKET_SIZE);
  addr_size = sizeof(client_addr);
  recvfrom(server_sockfd, buffer, MAX_PACKET_SIZE, 0,
           (struct sockaddr *)&client_addr, &addr_size);
  LOG(INFO) << "***Request received is: " << buffer;
  cli_address_ = client_addr;
  return buffer;
}

void UdpServer::send_data_segment(DataSegment *data_segment) {
  char *datagramChars = data_segment->SerializeToCharArray();
  sendto(sockfd_, datagramChars, MAX_PACKET_SIZE, 0,
         (struct sockaddr *)&cli_address_, sizeof(cli_address_));
  free(datagramChars);
}
}  // namespace safe_udp