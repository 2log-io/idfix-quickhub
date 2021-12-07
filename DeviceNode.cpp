#include "DeviceNode.h"
#include "DeviceNodeEventHandler.h"

extern "C"
{
	#include "esp_log.h"
	#include <freertos/FreeRTOS.h>
	#include <freertos/task.h>
}

#define DeviceNodeLogTAG		"DeviceNode"

namespace _2log
{

	DeviceNode::DeviceNode(IConnection *connection, DeviceNodeEventHandler *eventHandler, const std::string &nodeType, const std::string &id, const std::string &shortID, const uint32_t authKey)
		: _connection(connection), _nodeType(nodeType), _id(id), _shortID(shortID), _authKey(authKey), _eventHandler(eventHandler)
	{
		_connection->setConnectionEventHandler(this);
	}

	DeviceNode::~DeviceNode()
	{
		delete _connection;
	}

	bool DeviceNode::setDeviceNodeEventHandler(DeviceNodeEventHandler *newEventHandler)
	{
		_eventHandler = newEventHandler;
		return true;
	}

	bool DeviceNode::connect(uint32_t delayTime)
	{
		return _connection->connect(delayTime);
	}

	bool DeviceNode::disconnect()
	{
		return _connection->disconnect();
	}

	void DeviceNode::registerInitPropertiesCallback(DeviceNode::jsonCallbackFunction callbackFunction)
	{
		_initPropertiesCallback = callbackFunction;
	}

	void DeviceNode::registerRPC(const char *name, jsonCallbackFunction callback)
	{
		ESP_LOGD(DeviceNodeLogTAG, "DeviceNode::registerRPC");
		_rpcCallbacks[name] = callback;
	}

	bool DeviceNode::sendData(const char *subject)
	{
		ESP_LOGV(DeviceNodeLogTAG, "DeviceNode::sendData: %s running in Task %s", subject, pcTaskGetTaskName(NULL) );

		cJSON *payload = cJSON_CreateObject();

		if (cJSON_AddStringToObject(payload, "cmd", "msg") == nullptr)
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddStringToObject failed - cmd");
			cJSON_Delete(payload);
			return false;
		}

		cJSON *params = cJSON_AddObjectToObject(payload, "params");

		if ( params == nullptr )
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddObjectToObject failed - params");
			cJSON_Delete(payload);
			return false;
		}

		if (cJSON_AddStringToObject(params, "subject", subject) == nullptr)
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddStringToObject failed - subject");
			cJSON_Delete(payload);
			return false;
		}

		bool success = _connection->sendPayload(payload);

		cJSON_Delete(payload);

		return success;
	}

	void DeviceNode::connected()
	{
        _isConnected = true;
		cJSON *registerObject = cJSON_CreateObject();

		registerNode(registerObject);

		cJSON_Delete(registerObject);

		if ( _eventHandler )
		{
			_eventHandler->deviceNodeConnected();
		}
	}

	void DeviceNode::disconnected()
	{
        _isConnected = false;
		if ( _eventHandler )
		{
			_eventHandler->deviceNodeDisconnected();
		}
	}

	void DeviceNode::jsonReceived(const cJSON* jsonMessage)
	{
		ESP_LOGV(DeviceNodeLogTAG, "jsonReceived() running in Task %s", pcTaskGetTaskName(NULL) );

		cJSON *cmdItem = cJSON_GetObjectItemCaseSensitive(jsonMessage, "cmd");

		if ( ! ( cJSON_IsString(cmdItem) && cmdItem->valuestring != nullptr ) )
		{
			ESP_LOGE(DeviceNodeLogTAG, "Json message does not contain a command");
			return;
		}

		std::string command = cmdItem->valuestring;

		if ( command == "call" )
		{

			cJSON *paramsItem = cJSON_GetObjectItemCaseSensitive(jsonMessage, "params");

			if ( paramsItem == nullptr || ! cJSON_IsObject(paramsItem) )
			{
				ESP_LOGE(DeviceNodeLogTAG, "failed to get params");
				return;
			}

			cJSON *argumentsObject = paramsItem->child;
			if ( argumentsObject == nullptr || ! cJSON_IsObject(argumentsObject) )
			{
				ESP_LOGE(DeviceNodeLogTAG, "failed to get RPC arguments");
				return;
			}

			const char *rpcFunction = argumentsObject->string;

			if ( rpcFunction == nullptr )
			{
				ESP_LOGE(DeviceNodeLogTAG, "failed to get RPC function name");
				return;
			}

			callRPC(rpcFunction, argumentsObject);

			return;
		}

		if ( command == "init" )
		{
			return;
		}

		if ( command == "setkey" )
		{
			cJSON *paramsItem = cJSON_GetObjectItemCaseSensitive(jsonMessage, "params");

			if ( cJSON_IsNumber(paramsItem) )
			{
				uint32_t authKey = static_cast<uint32_t>(paramsItem->valueint);
				ESP_LOGD(DeviceNodeLogTAG, "Got authentication key: %u", authKey);

				if ( _eventHandler )
				{
					_eventHandler->deviceNodeAuthKeyChanged(authKey);
				}
			}
			else
			{
				ESP_LOGE(DeviceNodeLogTAG, "params for setkey is not a number");
			}

			return;
		}
	}

	void DeviceNode::callRPC(const char *name, cJSON* argument)
	{
		ESP_LOGD(DeviceNodeLogTAG, "DeviceNode::callRPC(%s)", name);

		jsonCallbackFunction callback = _rpcCallbacks[name];

		if ( callback )
		{
			callback(argument);
		}
		else
		{
			ESP_LOGE(DeviceNodeLogTAG, "NOT A VALID CALLBACK");
		}
	}

	void DeviceNode::registerNode(cJSON *registerObject)
	{
		ESP_LOGD(DeviceNodeLogTAG, "DeviceNode::registerNode()");

		if (cJSON_AddStringToObject(registerObject, "command", "node:register") == nullptr)
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddStringToObject failed - command: \"node:register\"");
			return;
		}

		cJSON *parametersObject = cJSON_AddObjectToObject(registerObject, "parameters");

		if ( parametersObject == nullptr || ! cJSON_IsObject(parametersObject) )
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddObjectToObject failed - creating parameters object");
			return;
		}

		cJSON *functionsArray = cJSON_AddArrayToObject(parametersObject, "functions");
		if ( functionsArray == nullptr || ! cJSON_IsArray(functionsArray) )
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddArrayToObject failed - creating functions array");
			return;
		}

		for ( auto callback : _rpcCallbacks )
		{
			cJSON *callbackObject = cJSON_CreateObject();

			if ( callbackObject != nullptr && cJSON_IsObject(callbackObject) )
			{
				if (cJSON_AddStringToObject(callbackObject, "name", callback.first ) != nullptr)
				{
					cJSON_AddItemToArray(functionsArray, callbackObject);
				}
				else
				{
					cJSON_Delete(callbackObject);
				}
			}
		}

		if (cJSON_AddStringToObject(parametersObject, "id", _id.c_str() ) == nullptr)
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddStringToObject failed - id");
			return;
		}

		if (cJSON_AddNumberToObject(parametersObject, "key", _authKey ) == nullptr)
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddNumberToObject failed - key");
			return;
		}

		if (cJSON_AddStringToObject(parametersObject, "sid", _shortID.c_str() ) == nullptr)
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddStringToObject failed - sid: %s", _shortID.c_str() );
			return;
		}

		if (cJSON_AddStringToObject(parametersObject, "type", _nodeType.c_str() ) == nullptr)
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddStringToObject failed - type");
			return;
		}


		if ( _initPropertiesCallback )
		{
			cJSON *propertiesObject = cJSON_AddObjectToObject(parametersObject, "properties");

			if ( propertiesObject == nullptr || ! cJSON_IsObject(propertiesObject) )
			{
				ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddObjectToObject failed - creating properties object");
			}
			else
			{
				_initPropertiesCallback(propertiesObject);
			}
		}

		_connection->sendPayload(registerObject);
	}

	void DeviceNode::setProperty(const char *property, int value)
	{
        if ( !_isConnected )
        {
            return;
        }

		ESP_LOGD(DeviceNodeLogTAG, "DeviceNode::setProperty()");
		cJSON *parametersObject = cJSON_CreateObject();

		if ( cJSON_AddNumberToObject(parametersObject, property, value) == nullptr )
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddNumberToObject failed - property: %s", property);
			cJSON_Delete(parametersObject);
			return;
		}

		setProperties(parametersObject);

		cJSON_Delete(parametersObject);
	}


    void DeviceNode::setProperty(const char *property, const char* value)
    {
        if ( !_isConnected )
        {
            return;
        }

        ESP_LOGD(DeviceNodeLogTAG, "DeviceNode::setProperty()");
        cJSON *parametersObject = cJSON_CreateObject();

        if ( cJSON_AddStringToObject(parametersObject, property, value) == nullptr )
        {
            ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddNumberToObject failed - property: %s", property);
            cJSON_Delete(parametersObject);
            return;
        }

        setProperties(parametersObject);

        cJSON_Delete(parametersObject);
    }

    void DeviceNode::setProperty(const char *property, bool value)
    {
        if ( !_isConnected )
        {
            return;
        }

        ESP_LOGI(DeviceNodeLogTAG, "DeviceNode::setProperty()");
        cJSON *parametersObject = cJSON_CreateObject();

        if ( cJSON_AddBoolToObject(parametersObject, property, value) == nullptr )
        {
            ESP_LOGI(DeviceNodeLogTAG, "cJSON_AddBoolToObject failed - property: %s", property);
            cJSON_Delete(parametersObject);
            return;
        }

        setProperties(parametersObject);

        cJSON_Delete(parametersObject);
    }

    void DeviceNode::setProperty(const char *property, float value)
    {
        if ( !_isConnected )
        {
            return;
        }

        ESP_LOGD(DeviceNodeLogTAG, "DeviceNode::setProperty()");
        cJSON *parametersObject = cJSON_CreateObject();

        if ( cJSON_AddNumberToObject(parametersObject, property, value) == nullptr )
        {
            ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddNumberToObject failed - property: %s", property);
            cJSON_Delete(parametersObject);
            return;
        }

        setProperties(parametersObject);

        cJSON_Delete(parametersObject);
    }

	void DeviceNode::setProperties(cJSON *parameters)
	{
        ESP_LOGI(DeviceNodeLogTAG, "DeviceNode::setProperties()");
		cJSON *payload = cJSON_CreateObject();

		if ( cJSON_AddStringToObject(payload, "cmd", "set") == nullptr )
		{
			ESP_LOGE(DeviceNodeLogTAG, "cJSON_AddStringToObject(payload, \"cmd\", \"set\") failed");
			cJSON_Delete(payload);
			return;
		}

		// add payload as reference(!) so it won't be deleted by cJSON_Delete
		// payload is owned by caller and should be deleted there
		cJSON_AddItemReferenceToObject(payload, "params", parameters);

		_connection->sendPayload(payload);

		cJSON_Delete(payload);
	}

}
