/*
 * httpserver.h
 *
 *  Created on: 25 Jul 2013
 *      Author: gnuvebox
 */

#include "mongcpp.h"
using namespace mongoose;

#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

class HttpServer: public MongooseServer {
public:
	HttpServer(): MongooseServer() {}
    virtual ~HttpServer() {}
protected:
    virtual bool handleEvent(ServerHandlingEvent eventCode, MongooseConnection &connection, const MongooseRequest &request, MongooseResponse &response) {
        bool res = false;

        if (eventCode == MG_NEW_REQUEST) {
            if (request.getUri() == string("/info")) {
                handleInfo(request, response);
                res = true;
            }
        }

        return res;
    }

    void handleInfo(const MongooseRequest &request, MongooseResponse &response) {
        response.setStatus(200);
        response.setConnectionAlive(false);
        response.setCacheDisabled();
        response.setContentType("text/html");
        response.addContent(generateInfoContent(request));
        response.write();
    }

    const string generateInfoContent(const MongooseRequest &request) {
        string result;
        result = "<h1>Sample Info Page</h1>";
        result += "<br />Request URI: " + request.getUri();
        result += "<br />Your IP: " + ipToString(request.getRemoteIp());

	time_t tim;
	time(&tim);

        result += "<br />Current date & time: " + toString(ctime(&tim));
        result += "<br /><br /><a href=\"/\">Index page</a>";

        return result;
    }
};


#endif /* HTTPSERVER_H_ */
