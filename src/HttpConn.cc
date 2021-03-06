#include "HttpConn.h"

#include <assert.h>
#include <unistd.h>

#include "Logger.h"

using namespace tiny;

const string HttpConn::src_dir = string(getcwd(nullptr, 256)).append("/pages/");
atomic<int> HttpConn::conns;

HttpConn::HttpConn(int fd, const sockaddr_in &addr)
    : fd_(fd),
      addr_(addr),
      isClose_(true),
      read_buff_(),
      write_buff_(),
      request_(),
      response_() {}

HttpConn::~HttpConn() { Close(); }

void HttpConn::Init(int fd, const sockaddr_in &addr) {
  assert(fd > 0);
  ++conns;
  addr_ = addr;
  fd_ = fd;
  isClose_ = false;
}

ssize_t HttpConn::Read(int &saveError) {
  char buff[65535];
  ssize_t len = read(fd_, &buff, sizeof(buff));
  if (len <= 0) {
    saveError = errno;
    return len;
  }
  read_buff_.clear();
  read_buff_.append(buff);
  return len;
}

ssize_t HttpConn::Write(int &saveError) {
  ssize_t len = write(fd_, write_buff_.data(), write_buff_.size());
  if (len < 0) {
    saveError = errno;
    return len;
  } else if (len >= write_buff_.size()) {
    write_buff_.clear();
    return 0;
  } else {
    write_buff_ = write_buff_.substr(len + 1);
  }
  return len;
}

void HttpConn::Close() {
  response_.UnmapFile();
  if (isClose_ == false) {
    isClose_ = true;
    --conns;
    close(fd_);
  }
}

bool HttpConn::Process() {
  request_.Init();
  if (read_buff_.size() <= 0) {
    return false;
  } else if (request_.Parse(read_buff_)) {
    response_.Init(src_dir, request_.GetPath(), request_.GetIsKeepAlive(), 200);
  } else {
    response_.Init(src_dir, request_.GetPath(), false, 400);
  }
  response_.MakeResponse(write_buff_);
  return true;
}

int HttpConn::GetFd() const { return fd_; };

struct sockaddr_in HttpConn::GetAddr() const {
  return addr_;
}

string HttpConn::GetIP() const { return string(inet_ntoa(addr_.sin_addr)); }

int HttpConn::GetPort() const { return addr_.sin_port; }

bool HttpConn::GetIsKeepAlive() const { return request_.GetIsKeepAlive(); }
