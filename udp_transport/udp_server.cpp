#include "udp_server.h"

namespace safe_udp {
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
}  // namespace safe_udp