/*
 * httpserver.cpp
 *
 *  Created on: 25 Jul 2013
 *      Author: gnuvebox
 */

#include "HttpServer.h"


HttpServer::HttpServer(): MongooseServer()
{
	_pluinHost=0;
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
	_pluinHost = pluginHost;

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

	// if this is a post, get the post data
	if (request.getRequestMethod() == "POST")
	{
		// TODO: get post data into json object
	}
	else
	{
		// make an empty json object
		json = cJSON_CreateObject();
	}

	// now act depending on the path
	if (uri == string("/getallplugins"))
	{
		_pluinHost->getAvailablePlugins(json);
	} else if (uri == string("/addplugin")) {

	}
	void addPlugin(cJSON* json);
	void swapPlugin(cJSON* json);

	void setPluginParams(cJSON* json);

	// did we make any json?
	if (json->child)
	{
		// output the json
		char * content = cJSON_Print(json);
		response.setStatus(200);
		response.setConnectionAlive(false);
		response.setCacheDisabled();
		response.setContentType("application/json");
		response.addContent(content);
		response.write();
		free(content);
	}
	else
	{
		// no json: error code for you.
		response.setStatus(404);
		response.setConnectionAlive(false);
		response.write();
	}

	// clean up
	cJSON_Delete(json);

	return res;
}
