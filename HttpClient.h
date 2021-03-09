/*******************************************************************************
* Copyright (C) 2021 - 2026, winsoft666, <winsoft666@outlook.com>.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
*
* Expect bugs
*
* Please use and enjoy. Please let me know of any bugs/improvements
* that you have found/implemented and I will fix/incorporate them into this
* file.
*******************************************************************************/

#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_
#pragma once

#include <string>
#include <map>
#include <future>

namespace Http {
using Headers = std::multimap<std::string, std::string>;

// HTTP Request Datagram
class RequestDatagram {
 public:
  enum class METHOD {
    GET = 0,
    POST,
    HEAD,
    DEL, // DELETE conflict with winnt.h DELETE macro.
    PUT,
    PATCH,
    OPTIONS,
    TRACE,
    CONNECT
  };
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

// HTTP Response Datagram
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

// Http Client
class Client {
 public:
  using RequestResult =
      std::function<void(int,  // >=0 is CURLcode, <0 is custom error codes.
                         const ResponseDatagram&  // HTTP response datagram
                         )>;
  // Custom error code
  enum : int { NO_CURL_HANDLE = -1, USER_ABORT = -2 };

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
  // Issue an asynchronous HTTP request.
  // Return value:
  //   The return value does not indicate that the request is finished.
  //   Return false only when has another request is doing, other case return true.
  //
  bool Request(const RequestDatagram& reqDg,
               RequestResult ret,
               int connectionTimeout = 5000,  // ms
               int retry = 0);

  // Abort the current HTTP request.
  //
  void Abort();

  // Wait for the current HTTP request to be finished.
  // Return value:
  //   true  - request has finished or request finished with the special time(ms).
  //   false - timeout, the request did not finished with the special time(ms).
  //
  bool Wait(int ms);

  // Set HTTP proxy
  void SetProxy(const std::string& proxy);
  std::string GetProxy() const;

 private:
  int DoRequest(const RequestDatagram& reqDg,
                ResponseDatagram& rspDg,
                int connectionTimeout,
                int retry);

 private:
  static bool initialized_;
  void* curl_;
  std::future<void> future_;
  std::atomic_bool abort_;
  std::string proxy_;
};
}  // namespace Http
#endif  // !HTTP_CLIENT_H_