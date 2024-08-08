#include "udp_client.h"

#include <netdb.h>

#include <string>

#include <glog/logging.h>

namespace safe_udp {
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