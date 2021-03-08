#include <iostream>
#include "HttpClient.h"

using namespace Http;

int main() {
  Client::Cleanup cleanup;
  Client client;

  RequestDatagram reqDatagram;
  reqDatagram.SetMethod(RequestDatagram::METHOD::GET);
  reqDatagram.SetUrl("https://www.baidu.com");

  client.Request(reqDatagram,
                 [](int ccode, const ResponseDatagram& rspDg) {
                   std::cout << "Result: " << ccode << std::endl;
                   std::cout << "Http Code: " << rspDg.GetCode() << std::endl;
                   std::cout << "Headers:" << std::endl;
                   for (auto & item : rspDg.GetHeaders()) {
                       std::cout << item.first << ": " << item.second << std::endl;
                   }
                   std::cout << "Body Size:" << rspDg.GetBodySize() << std::endl;
                 },
                 3000, 1);

  std::cout << "Wait result: " << client.Wait(5000) << std::endl;

  return 0;
}
