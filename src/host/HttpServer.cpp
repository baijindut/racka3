/*
 * httpserver.cpp
 *
 *  Created on: 25 Jul 2013
 *      Author: gnuvebox
 */

#include "HttpServer.h"


HttpServer::HttpServer(): MongooseServer()
{
	_pluginHost=0;
}

HttpServer::~HttpServer()
{
}

bool HttpServer::handleEvent(ServerHandlingEvent eventCode,
								 MongooseConnection &connection,
								 const MongooseRequest &request,
								 MongooseResponse &response)
{
	bool res = false;

	switch (eventCode)
	{
	case MG_NEW_REQUEST:
		res = handleNewRequest(eventCode,connection,request,response);
		break;

	default:
		break;
	}

	return res;
}

void HttpServer::setPluginHost(Host* pluginHost)
{
	_pluginHost = pluginHost;

	setOption("document_root","www");
	start();
}

bool HttpServer::handleNewRequest(ServerHandlingEvent eventCode,
									  MongooseConnection &connection,
									  const MongooseRequest &request,
									  MongooseResponse &response)
{
	bool res = false;
	cJSON* json = 0;
	string uri = request.getUri();

	printf("Http %s %s\n",request.getRequestMethod().c_str(),uri.c_str());

	// if this is a post, get the post data
	if (request.getRequestMethod() == "POST")
	{
		std::string buffer;
		char c;

		while (connection.read(&c,1))
		{
			buffer.push_back(c);
		}

		printf("received POST:\n%s\n",buffer.c_str());
		json = cJSON_Parse(buffer.c_str());
	}
	else
	{
		// make an empty json object
		json = cJSON_CreateObject();
	}

	// we should have the POSTED json, or an empty json object
	if (json)
	{
		// now act depending on the path
		if (uri == string("/getallplugins"))
		{
			_pluginHost->getAvailablePlugins(json);
		} else if (uri == string("/addplugin")) {
			_pluginHost->addPlugin(json);
		} else if (uri == string("/removeplugin")) {
			// TODO
		} else if (uri == string("/swapPlugin")) {
			// TODO
		} else if (uri == string("/setpluginparams")) {
			// TODO
		} else if (uri == string("/getpluginparams")) {
			// TODO
		}

	}

	// did we make any json?
	if (json && json->child)
	{
		// output the json
		char * content = cJSON_Print(json);
		response.setStatus(200);
		response.setContentType("application/json");
		response.addContent(content);
		free(content);
	}
	else if (json)
	{
		// no json: error code for you.
		response.setStatus(404);
	}
	else
	{
		// no json: error code for you.
		response.setStatus(400);
	}

	response.setCacheDisabled();
	response.setConnectionAlive(false);
	response.write();

	// clean up
	cJSON_Delete(json);

	return res;
}
