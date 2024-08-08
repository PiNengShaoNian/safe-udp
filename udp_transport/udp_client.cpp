#include "udp_client.h"

#include <netdb.h>

#include <fstream>
#include <string>

#include <glog/logging.h>

namespace safe_udp {
UdpClient::UdpClient() {
  last_in_order_packet_ = -1;
  last_packet_received_ = -1;
  fin_flag_received_ = false;
}

void UdpClient::SendFileRequest(const std::string &file_name) {
  int n;
  int next_seq_expected;
  int segments_in_between = 0;
  initial_seq_number_ = 67;
  if (receiver_window_ == 0) {
    receiver_window_ = 100;
  }
  unsigned char *buffer =
      (unsigned char *)calloc(MAX_PACKET_SIZE, sizeof(unsigned char));
  LOG(INFO) << "server_add::" << server_address_.sin_addr.s_addr;
  LOG(INFO) << "server_add_port::" << server_address_.sin_port;
  LOG(INFO) << "server_add_family::" << server_address_.sin_family;
  n = sendto(sockfd_, file_name.c_str(), file_name.size(), 0,
             (struct sockaddr *)&(server_address_), sizeof(struct sockaddr_in));
  if (n < 0) {
    LOG(ERROR) << "Failed to write to socket !!!";
  }
  memset(buffer, 0, MAX_DATA_SIZE);

  std::fstream file;
  std::string file_path = std::string(CLIENT_FILE_PATH) + file_name;
  file.open(file_path.c_str(), std::ios::out);

  while ((n = recvfrom(sockfd_, buffer, MAX_PACKET_SIZE, 0, NULL, NULL)) > 0) {
    char buffer2[20];
    memcpy(buffer2, buffer, 20);
    if (strstr("FILE NOT FOUND", buffer2) != NULL) {
      LOG(ERROR) << "File not found !!!";
      return;
    }
  }
}

void UdpClient::CreateSocketAndServerConnection(
    const std::string &server_address, const std::string &port) {
  struct hostent *server;
  struct sockaddr_in server_address_;
  int sfd;
  int port_num;
  port_num = atoi(port.c_str());
  sfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sfd < 0) {
    LOG(ERROR) << "Failed to socket !!!";
  }
  server = gethostbyname(server_address.c_str());
  if (server == NULL) {
    LOG(ERROR) << "No such host !!!";
    exit(0);
  }

  memset(&server_address_, 0, sizeof(server_address_));
  server_address_.sin_family = AF_INET;
  memcpy(&server_address_.sin_addr.s_addr, server->h_addr, server->h_length);
  server_address_.sin_port = htons(port_num);

  sockfd_ = sfd;
  this->server_address_ = server_address_;
}
}  // namespace safe_udp