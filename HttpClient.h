#ifndef HTTP_HELPER_H_
#define HTTP_HELPER_H_
#pragma once

#include <string>
#include <map>
#include <future>

namespace Http {
typedef std::multimap<std::string, std::string> Headers;

class RequestDatagram {
 public:
  enum class METHOD { GET, POST, HEADER };
  RequestDatagram();
  RequestDatagram(const RequestDatagram& that);
  RequestDatagram& operator=(const RequestDatagram& that);
  ~RequestDatagram();

  METHOD GetMethod() const;
  void SetMethod(METHOD m);

  Headers GetHeaders() const;
  void SetHeaders(const Headers& headers);

  std::string GetUrl() const;
  void SetUrl(const std::string& url);

  const void* GetBodyData() const;
  size_t GetBodySize() const;
  void SetBody(const void* data, size_t dataSize);

 private:
  METHOD method_;
  Headers headers_;
  void* bodyData_;
  size_t bodySize_;
  std::string url_;
};

class ResponseDatagram {
 public:
  ResponseDatagram();
  ~ResponseDatagram();
  ResponseDatagram(const ResponseDatagram& that);
  ResponseDatagram& operator=(const ResponseDatagram& that);

  int GetCode() const;
  void SetCode(int code);

  Headers GetHeaders() const;
  void AddHeader(const std::string& key, const std::string& value);

  void* GetBody() const;
  size_t GetBodySize() const;
  void SetBody(void* data, size_t dataSize);

  size_t GetBodyCapacity() const;
  void SetBodyCapacity(size_t s);

 private:
  int code_;
  Headers headers_;
  void* bodyData_;
  size_t bodySize_;
  size_t bodyCapacity_;
};

bool IsHttps(const std::string& url);

class Client {
 public:
  using RequestResult = std::function<void(int, const ResponseDatagram&)>;
  Client();
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  virtual ~Client();

 public:
  static void Initialize();
  static void Uninitialize();

  class Cleanup {
   public:
    Cleanup();
    ~Cleanup();
  };

 public:
  // asynchronous http request
  bool Request(const RequestDatagram& reqDg, RequestResult ret, int connectionTimeout = 5000, int retry = 0);

  // abort http request
  void Abort();

  // wait http request finish
  // Return value:
  // true - request has been finished or request be finished within special time(ms).
  // false - timeout, request not be finished.
  bool Wait(int ms);

 private:
  int DoRequest(const RequestDatagram& reqDg, ResponseDatagram& rspDg, int connectionTimeout, int retry);

 private:
  static bool initialized_;
  void* curl_;
  std::future<void> future_;
  std::atomic_bool abort_;
};
}  // namespace Http
#endif  // !HTTP_HELPER_H_