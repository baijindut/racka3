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
	_soundInterface=0;
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
		// check that we want to handle this.
		if (request.getUri().substr(0,strlen("/racka3/")) == "/racka3/")
		{
			res = handleNewRequest(eventCode,connection,request,response);
		}
		break;

	default:
		break;
	}

	return res;
}

void HttpServer::setSoundInterface(SoundInterface* interface)
{
	_soundInterface=interface;
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
	bool handled = false;

	// if this is a post, get the post data
	if (request.getRequestMethod() == "POST")
	{
		std::string buffer;
		char c;

		while (connection.read(&c,1))
		{
			buffer.push_back(c);
		}

		json = cJSON_Parse(buffer.c_str());

		// if we couldnt parse the json? make empty json
		if (!json)
		{
			json = cJSON_CreateObject();
		}
	}
	else
	{
		// make an empty json object
		json = cJSON_CreateObject();
	}

	// now act depending on the path
	if (uri == string("/racka3/getallplugins"))					// <-- rack manipulation
	{
		_pluginHost->getAvailablePlugins(json);
	} else if (uri == string("/racka3/addplugin")) {
		_pluginHost->addPlugin(json);
	} else if (uri == string("/racka3/removeplugin")) {
		_pluginHost->removePlugin(json);
	} else if (uri == string("/racka3/moveplugin")) {
		_pluginHost->movePlugin(json);
	} else if (uri == string("/racka3/setparamvalue")) {
		_pluginHost->setPluginParam(json);
	} else if (uri == string("/racka3/getpluginchain")) {
		_pluginHost->getPluginChain(json);
	}
	else if (uri == string("/racka3/storePluginPreset")) {	// <-- plugin presets
		_pluginHost->storePluginPreset(json);
	} else if (uri == string("/racka3/deletePluginPreset")) {
		_pluginHost->deletePluginPreset(json);
	}
	else if (uri == string("/racka3/storeRackPreset")) {	// <-- rack presets
		_pluginHost->storeRackPreset(json);
	} else if (uri == string("/racka3/deleteRackPreset")) {
		_pluginHost->deleteRackPreset(json);
	} else if (uri == string("/racka3/loadRackPreset")) {
		_pluginHost->loadRackPreset(json);
	} else if (uri == string("/racka3/listRackPresets")) {
		_pluginHost->listRackPresets(json);
	}
	else if (uri == string("/racka3/listDevices")) {				// <-- soundcard configuration
		_soundInterface->listDevices(json);
	} else if (uri == string("/racka3/setDevice")) {
		// todo: check rack is empty before allowing this!
		if (_pluginHost->getPluginChainSize())
			cJSON_AddStringToObject(json,"error","cannot change sound settings unless rack is empty");
		else
			_soundInterface->init(json);
	} else if (uri == string("/racka3/getCurrentDevice")) {
		_soundInterface->getCurrent(json);
	}

	// did we make any json?
	if (json->child)
	{
		// output the json
		char * content = cJSON_Print(json);
		response.setStatus(200);
		response.setContentType("application/json");
		response.addContent(content);
		free(content);
	}
	else
	{
		// no json: error code for you.
		response.setStatus(404);
	}

	response.setCacheDisabled();
	response.setConnectionAlive(false);
	response.write();

	// clean up
	cJSON_Delete(json);

	return res;
}
