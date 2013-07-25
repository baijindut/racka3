/*
 * httpserver.cpp
 *
 *  Created on: 25 Jul 2013
 *      Author: gnuvebox
 */

#include "httpserver.h"


HttpServer::HttpServer(): MongooseServer()
{

}

HttpServer::~HttpServer()
{
}

bool HttpServer::handleEvent(ServerHandlingEvent eventCode, MongooseConnection &connection, const MongooseRequest &request, MongooseResponse &response) {
	bool res = false;

	if (eventCode == MG_NEW_REQUEST) {
		if (request.getUri() == string("/info")) {
			handleInfo(request, response);
			res = true;
		}
	}

	return res;
}

void HttpServer::handleInfo(const MongooseRequest &request, MongooseResponse &response) {
	response.setStatus(200);
	response.setConnectionAlive(false);
	response.setCacheDisabled();
	response.setContentType("text/html");
	response.addContent(generateInfoContent(request));
	response.write();
}

const string HttpServer::generateInfoContent(const MongooseRequest &request) {
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

