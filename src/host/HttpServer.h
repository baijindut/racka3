/*
 * httpserver.h
 *
 *  Created on: 25 Jul 2013
 *      Author: gnuvebox
 */

#include "Host.h"

#include "mongcpp.h"
using namespace mongoose;
using namespace std;

#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

class HttpServer: public MongooseServer
{
public:
	HttpServer();
    virtual ~HttpServer();

    void setPluginHost(Host* pluginHost);

protected:
    virtual bool handleEvent(ServerHandlingEvent eventCode,
    							MongooseConnection &connection,
    							const MongooseRequest &request,
    							MongooseResponse &response);
private:
    bool handleNewRequest(ServerHandlingEvent eventCode,
							 MongooseConnection &connection,
							 const MongooseRequest &request,
							 MongooseResponse &response);

    Host* _pluinHost;
};


#endif /* HTTPSERVER_H_ */
