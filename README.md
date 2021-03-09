# http-client
HTTP asynchronous request library based libcurl.

# Dependency
Only has one dependent library: `libcurl`.


# Qucik start
```c++
#include <iostream>
#include "HttpClient.h"

using namespace Http;

int main() {
  Client::Cleanup cleanup;
  Client client;

  //client.SetProxy("http://localhost:8888");

  RequestDatagram reqDatagram;
  reqDatagram.SetMethod(RequestDatagram::METHOD::GET);
  reqDatagram.SetUrl("https://www.baidu.com");
  reqDatagram.SetHeaders(
      {{"Content-Type", "text/html"}, {"Cache-Control", "no-cache"}});

  client.Request(
      reqDatagram,
      [](int ccode, const ResponseDatagram& rspDatagram) {
        std::cout << "- Result: " << ccode << std::endl;
        std::cout << std::endl << "[Response Datagram]" << std::endl;
        std::cout << "- Code: " << rspDatagram.GetCode() << std::endl;
        std::cout << "- Headers:" << std::endl;
        for (auto& item : rspDatagram.GetHeaders()) {
          std::cout << item.first << ": " << item.second << std::endl;
        }
        std::cout << std::endl;

        std::string strBody;
        if (rspDatagram.GetBody()) {
          // Since terminate buffer limit, only display 100 char.
          strBody.assign((const char*)rspDatagram.GetBody(), 100);
        }
        std::cout << "- Body total:" << rspDatagram.GetBodySize()
                  << "bytes, 0~100:" << std::endl
                  << strBody << std::endl
                  << "..." << std::endl;
      },
      3000, 1);

  bool waitRet = client.Wait(1000);
  std::cout << "Wait result: " << waitRet << std::endl;

  // If not finish, abort this request.
  if (!waitRet) {
    client.Abort();
    waitRet = client.Wait(-1);

    std::cout << "Wait result: " << waitRet << std::endl;
  }
  return 0;
}
```