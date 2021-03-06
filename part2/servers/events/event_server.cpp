#include "server_event_handler.h"				
#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <thrift/server/TServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include "gen-cpp/Message.h"

using namespace ::apache::thrift;				
using namespace ::apache::thrift::protocol;		
using namespace ::apache::thrift::transport; 		
using namespace ::apache::thrift::server; 			
using namespace ::apache::thrift::concurrency; 		
using boost::shared_ptr; 					
using boost::make_shared; 					

const char * msgs[] = {"Apache Thrift!!",
                       "Childhood is a short season",
                       "'Twas brillig"};
class MessageHandler : public MessageIf {			
public:
    MessageHandler() : msg_index(0) {;}
    virtual void motd(std::string& _return) override {
        _return = msgs[++msg_index%3];
    }
private:
    unsigned int msg_index;
};

int main(int argc, char **argv) {
  //Setup the socket server and the service processor and handler
  const int port = 8585;
  auto handler = make_shared<MessageHandler>();			
  auto proc = make_shared<MessageProcessor>(handler);		
  auto svr_trans = make_shared<TServerSocket>(port);		

  //Setup the protocol and layered transport factories		
  auto trans_fac = make_shared<TBufferedTransportFactory>();
  auto proto_fac = make_shared<TCompactProtocolFactory>();

  //Setup the thread manager and thread factory, then create the threads 
  auto t_man = ThreadManager::newSimpleThreadManager(2,1);		
  auto t_fac = make_shared<PlatformThreadFactory>();			
  t_man->threadFactory(t_fac);  						
  t_man->start(); 								

  //Setup the server and run it on a background thread
  TThreadPoolServer server(proc, svr_trans, trans_fac, proto_fac, t_man);
  server.setTimeout(3000);							
  server.setServerEventHandler(make_shared<PoolSvrEvtHandler>(t_man,2,4));
  std::thread server_thread(std::bind(&TThreadPoolServer::serve, &server));
										
  //Wait for the user to quit
  std::string str;								
  std::cout << "[Server:" << port << "] enter to quit" << std::endl;
  std::getline(std::cin, str);

  //Stop accepting new connections (thread manager stops when tasks end)
  server.stop();								
  std::cout   << "Waiting for current(" 
              << t_man->workerCount() - t_man->idleWorkerCount()
              << ") and queued(" << t_man->pendingTaskCount() 
              << ") client tasks to exit..." << std::endl;
  server_thread.join();							
  std::cout << "service complete, exiting." << std::endl;
}

