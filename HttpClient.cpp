#include "HttpClient.h"
#include <assert.h>
#include "curl/curl.h"

namespace Http {

//////////////////////////////////////////////////////////////////////////
// RequestDatagram
//////////////////////////////////////////////////////////////////////////

RequestDatagram::RequestDatagram() : method_(METHOD::GET), bodyData_(nullptr), bodySize_(0) {}

RequestDatagram::RequestDatagram(const RequestDatagram& that) {
  url_ = that.url_;
  method_ = that.method_;
  headers_ = that.headers_;
  bodySize_ = that.bodySize_;
  bodyData_ = nullptr;
  if (that.bodyData_ && that.bodySize_ > 0) {
    bodyData_ = malloc(that.bodySize_);
    if (bodyData_)
      memcpy(bodyData_, that.bodyData_, that.bodySize_);
  }
}

RequestDatagram& RequestDatagram::operator=(const RequestDatagram& that) {
  url_ = that.url_;
  method_ = that.method_;
  headers_ = that.headers_;
  if (bodyData_) {
    free(bodyData_);
    bodyData_ = nullptr;
  }
  bodySize_ = that.bodySize_;
  if (that.bodyData_ && that.bodySize_ > 0) {
    bodyData_ = malloc(that.bodySize_);
    if (bodyData_)
      memcpy(bodyData_, that.bodyData_, that.bodySize_);
  }

  return *this;
}

RequestDatagram::~RequestDatagram() {
  if (bodyData_) {
    free(bodyData_);
    bodyData_ = nullptr;
  }
  bodySize_ = 0;
}

RequestDatagram::METHOD RequestDatagram::GetMethod() const {
  return method_;
}

void RequestDatagram::SetMethod(METHOD m) {
  method_ = m;
}

Headers RequestDatagram::GetHeaders() const {
  return headers_;
}

void RequestDatagram::SetHeaders(const Headers& headers) {
  headers_ = headers;
}

std::string RequestDatagram::GetUrl() const {
  return url_;
}

void RequestDatagram::SetUrl(const std::string& url) {
  url_ = url;
}

const void* RequestDatagram::GetBodyData() const {
  return bodyData_;
}

size_t RequestDatagram::GetBodySize() const {
  return bodySize_;
}

void RequestDatagram::SetBody(const void* data, size_t dataSize) {
  if (bodyData_) {
    free(bodyData_);
    bodyData_ = NULL;
  }

  bodySize_ = 0;

  if (dataSize > 0) {
    bodyData_ = malloc(dataSize);
    if (bodyData_) {
      memcpy(bodyData_, data, dataSize);
      bodySize_ = dataSize;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// ResponseDatagram
//////////////////////////////////////////////////////////////////////////

ResponseDatagram::ResponseDatagram() : code_(0), bodyData_(nullptr), bodySize_(0), bodyCapacity_(0) {}

ResponseDatagram::ResponseDatagram(const ResponseDatagram& that) {
  code_ = that.code_;
  headers_ = that.headers_;
  bodySize_ = that.bodySize_;
  bodyCapacity_ = that.bodyCapacity_;
  bodyData_ = nullptr;
  if (that.bodyData_) {
    bodyData_ = malloc(that.bodyCapacity_);
    if (bodyData_)
      memcpy(bodyData_, that.bodyData_, that.bodySize_);
  }
}

ResponseDatagram& ResponseDatagram::operator=(const ResponseDatagram& that) {
  code_ = that.code_;
  headers_ = that.headers_;
  if (bodyData_) {
    free(bodyData_);
    bodyData_ = nullptr;
  }
  bodySize_ = that.bodySize_;
  bodyCapacity_ = that.bodyCapacity_;

  if (that.bodyData_) {
    bodyData_ = malloc(that.bodyCapacity_);
    if (bodyData_)
      memcpy(bodyData_, that.bodyData_, that.bodySize_);
  }

  return *this;
}

ResponseDatagram::~ResponseDatagram() {
  if (bodyData_) {
    free(bodyData_);
    bodyData_ = nullptr;
  }

  bodySize_ = 0;
  bodyCapacity_ = 0;
}

int ResponseDatagram::GetCode() const {
  return code_;
}

void ResponseDatagram::SetCode(int code) {
  code_ = code;
}

Headers ResponseDatagram::GetHeaders() const {
  return headers_;
}

void ResponseDatagram::AddHeader(const std::string& key, const std::string& value) {
  headers_.insert(std::make_pair(key, value));
}

void* ResponseDatagram::GetBody() const {
  return bodyData_;
}

size_t ResponseDatagram::GetBodySize() const {
  return bodySize_;
}

void ResponseDatagram::SetBody(void* data, size_t dataSize) {
  bodyData_ = data;
  bodySize_ = dataSize;
}

size_t ResponseDatagram::GetBodyCapacity() const {
  return bodyCapacity_;
}

void ResponseDatagram::SetBodyCapacity(size_t s) {
  bodyCapacity_ = s;
}

//////////////////////////////////////////////////////////////////////////
// Client
//////////////////////////////////////////////////////////////////////////

namespace {
const size_t kBodyBufferStep = 1024;
const size_t kHeaderBufferStep = 1024;
}  // namespace

Client::Cleanup::Cleanup() {
  Client::Initialize();
}

Client::Cleanup::~Cleanup() {
  Client::Uninitialize();
}

bool Client::initialized_ = false;

static size_t __BodyWriteCallback__(void* data, size_t size, size_t nmemb, void* userdata) {
  ResponseDatagram* rspDg = static_cast<ResponseDatagram*>(userdata);
  size_t total = size * nmemb;
  assert(rspDg);
  if (!rspDg) {
    return -1;
  }

  void* oldBodyData = rspDg->GetBody();
  size_t oldBodySize = rspDg->GetBodySize();
  size_t remainBufSize = rspDg->GetBodyCapacity() - oldBodySize;

  if (remainBufSize < total) {
    size_t newBufSize = oldBodySize + (total > kBodyBufferStep ? total * 2 : kBodyBufferStep);
    void* buf = malloc(newBufSize);
    assert(buf);
    if (!buf) {
      return -1;
    }

    if (oldBodyData && oldBodySize > 0)
      memcpy(buf, oldBodyData, oldBodySize);

    rspDg->SetBodyCapacity(newBufSize);
    rspDg->SetBody(buf, oldBodySize);
  }

  void* curBuf = rspDg->GetBody();
  memcpy(((char*)curBuf + oldBodySize), data, total);
  rspDg->SetBody(curBuf, oldBodySize + total);

  return total;
}

size_t __HeaderWriteCallback__(char* buffer, size_t size, size_t nitems, void* userdata) {
  ResponseDatagram* rspDg = static_cast<ResponseDatagram*>(userdata);
  assert(rspDg);
  if (!rspDg) {
    return -1;
  }

  size_t total = size * nitems;
  std::string header;
  header.assign(buffer, size * nitems);

  size_t pos = header.find(": ");

  if (pos == std::string::npos) {
    return total;
  }

  std::string key = header.substr(0, pos - 1);
  std::string value = header.substr(pos + 2, header.length() - pos - 4);

  rspDg->AddHeader(key, value);

  return total;
}

Client::Client(void) : curl_(nullptr) {
  abort_.store(false);
  curl_ = static_cast<void*>(curl_easy_init());
}

Client::~Client(void) {
  Abort();
  if (future_.valid()) {
    future_.get();
  }
  CURL* pCURL = static_cast<CURL*>(curl_);
  if (pCURL) {
    curl_easy_cleanup(static_cast<CURL*>(pCURL));
  }
  curl_ = nullptr;
}

void Client::Initialize() {
  if (!initialized_) {
    curl_global_init(CURL_GLOBAL_ALL);
    initialized_ = true;
  }
}

void Client::Uninitialize() {
  if (initialized_) {
    curl_global_cleanup();
    initialized_ = false;
  }
}

bool Client::Request(const RequestDatagram& reqDg, RequestResult reqRet, int connectionTimeout /*= 5000*/, int retry /*= 0*/) {
  if (future_.valid() && future_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
    return false;
  }
  abort_.store(false);
  future_ = std::async(std::launch::async, [=]() {
    ResponseDatagram rspDg;
    int ccode = DoRequest(reqDg, rspDg, connectionTimeout, retry);
    if (reqRet) {
      reqRet(ccode, rspDg);
    }
  });

  return true;
}

int Client::DoRequest(const RequestDatagram& reqDg, ResponseDatagram& rspDg, int connectionTimeout, int retry) {
  CURL* pCURL = static_cast<CURL*>(curl_);
  if (!pCURL) {
    return -1;
  }
  curl_easy_setopt(pCURL, CURLOPT_URL, reqDg.GetUrl().c_str());
  curl_easy_setopt(pCURL, CURLOPT_HEADER, 1);
  curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT_MS, connectionTimeout);
  curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYHOST, 0L);

  switch (reqDg.GetMethod()) {
    case RequestDatagram::METHOD::GET:
      curl_easy_setopt(pCURL, CURLOPT_POST, 0);
      break;
    case RequestDatagram::METHOD::POST:
      curl_easy_setopt(pCURL, CURLOPT_POST, 1);
      break;
    case RequestDatagram::METHOD::HEADER:
      curl_easy_setopt(pCURL, CURLOPT_NOBODY, 1);
      break;
    default:
      assert(false);
      return -2;
  }

  struct curl_slist* headerChunk = nullptr;
  const Headers& headers = reqDg.GetHeaders();
  if (headers.size() > 0) {
    for (auto& it : headers) {
      std::string headerStr = it.first + ": " + it.second;
      headerChunk = curl_slist_append(headerChunk, headerStr.c_str());
    }
    curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, headerChunk);
  }

  if (reqDg.GetBodySize() > 0 && reqDg.GetBodyData()) {
    curl_easy_setopt(pCURL, CURLOPT_POSTFIELDS, reqDg.GetBodyData());
    curl_easy_setopt(pCURL, CURLOPT_POSTFIELDSIZE, reqDg.GetBodySize());
  }

  curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, __BodyWriteCallback__);
  curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, (void*)&rspDg);

  curl_easy_setopt(pCURL, CURLOPT_HEADERFUNCTION, __HeaderWriteCallback__);
  curl_easy_setopt(pCURL, CURLOPT_HEADERDATA, (void*)&rspDg);

  CURLcode ccode = CURL_LAST;
  do {
    ccode = curl_easy_perform(pCURL);
    if (ccode == CURLE_OK || ccode != CURLE_OPERATION_TIMEDOUT)
      break;
  } while (retry-- > 0);

  if (headerChunk) {
    curl_slist_free_all(headerChunk);
    headerChunk = nullptr;
  }

  if (ccode != CURLE_OK) {
    if (abort_.load())
      return -4;
    return ccode;
  }

  int rspCode = 0;
  if (CURLE_OK == curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &rspCode)) {
    rspDg.SetCode(rspCode);
  }

  if (headerChunk) {
    curl_slist_free_all(headerChunk);
    headerChunk = nullptr;
  }

  return ccode;
}

void Client::Abort() {
  abort_.store(true);
  CURL* pCURL = static_cast<CURL*>(curl_);
  if (pCURL) {
    curl_easy_setopt(pCURL, CURLOPT_TIMEOUT_MS, 1);
  }
}

bool Client::Wait(int ms) {
  if (!future_.valid())
    return true;
  return future_.wait_for(std::chrono::milliseconds(ms)) == std::future_status::ready;
}

bool IsHttps(const std::string& url) {
  if (url.substr(0, 6) == "https:")
    return true;
  return false;
}

}  // namespace Http