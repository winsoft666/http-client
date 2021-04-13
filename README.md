# http-client
HTTP asynchronous request library based libcurl.

# Dependency
Only has one dependent library: [libcurl](https://curl.se/libcurl/).


# Qucik start
```c++
#include <iostream>
#include "HttpClient.h"

using namespace Http;

int main() {
  Client::Cleanup cleanup;
  Client client;

  //client.SetProxy("http://localhost:8888");

  std::shared_ptr<RequestDatagram> reqDatagram =
      std::make_shared<RequestDatagram>();
  reqDatagram->SetMethod(RequestDatagram::METHOD::GET);
  reqDatagram->SetUrl("https://github.com/winsoft666");
  reqDatagram->SetHeaders(
      {{"Content-Type", "text/html"}, {"Cache-Control", "no-cache"}});

  if (!client.Request(
          reqDatagram,  // request datagram
          [](int ccode, const ResponseDatagram& rspDatagram) {
            std::cout << "Result: " << ccode << std::endl;
            std::cout << std::endl << "------ [Response Datagram] ------\n";
            std::cout << "<<<<<<  Code   >>>>>> \n"
                      << rspDatagram.GetCode() << "\n\n";
            std::cout << "<<<<<< Headers >>>>>>\n";
            for (auto& item : rspDatagram.GetHeaders()) {
              std::cout << item.first << ": " << item.second << std::endl;
            }
            std::cout << "\n\n";

            std::string strBody;
            if (rspDatagram.GetBody()) {
              strBody.assign((const char*)rspDatagram.GetBody(),
                             rspDatagram.GetBodySize());
            }
            std::cout << "<<<<<< Body (" << rspDatagram.GetBodySize()
                      << "bytes) >>>>>>\n"
                      << strBody << "\n...\n";
          },
          true,  // allow redirect
          3000,  // connection timeout
          1      // retry times
          )) {
    std::cout << "Current Client object is already requesting!" << std::endl;
    return 1;
  }

  bool waitRet = client.Wait(-1);
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