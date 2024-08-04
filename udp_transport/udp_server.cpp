#include "udp_server.h"

#include <arpa/inet.h>

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
}  // namespace safe_udp