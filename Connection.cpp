#include "Connection.h"
#include "auxiliary.h"
#include "IDFixTask.h"

extern "C"
{
	#include "esp_log.h"
}

#include "BuildConfig.h"

namespace
{
    const char *LOG_TAG = "2log::Connection";
}

namespace _2log
{

	Connection::Connection(const std::string &url, const char *caCertificate) : _serverURL(url), _connectionID(0), _connected(false), _webSocket(this), _eventHandler(nullptr)
	{
        _webSocket.start();
        _webSocket.setURL(url);

        if ( caCertificate )
        {
            _webSocket.setCaCertificate(caCertificate);
        }
	}

	bool Connection::connect(uint32_t delayTime)
	{
		if ( ! _webSocket.connect(delayTime) )
		{
            ESP_LOGE(LOG_TAG, "_webSocket.connect() failed");
			return false;
		}

		return true;
	}

	bool Connection::disconnect()
	{
		return _webSocket.disconnect();
	}

	bool Connection::sendPayload(const cJSON *payload)
	{
		if ( ! _connected )
		{
            ESP_LOGE(LOG_TAG, "Connection::sendPayload: not connected");
			return false;
		}

		if ( payload == nullptr || cJSON_IsInvalid(payload) )
		{
            ESP_LOGE(LOG_TAG, "Connection::sendPayload: payload invalid");
            return false;
		}

		cJSON *envelope = cJSON_CreateObject();

		if (cJSON_AddStringToObject(envelope, "command", "send") == nullptr)
		{
            ESP_LOGE(LOG_TAG, "cJSON_AddStringToObject failed");
			cJSON_Delete(envelope);
			return false;
		}

		if (cJSON_AddNumberToObject(envelope, "uuid", _connectionID) == nullptr)
		{
            ESP_LOGE(LOG_TAG, "cJSON_AddNumberToObject failed");
			cJSON_Delete(envelope);
			return false;
		}

		// add payload as reference(!) so it won't be deleted by cJSON_Delete
		// payload is owned by caller and should be deleted there
		cJSON_AddItemReferenceToObject(envelope, "payload", const_cast<cJSON*>(payload) );

		sendJSON(envelope);
		cJSON_Delete(envelope);

		return true;
	}

	bool Connection::setConnectionEventHandler(ConnectionEventHandler *newEventHandler)
	{
		_eventHandler = newEventHandler;
		return true;
	}

	bool Connection::sendJSON(const cJSON *json)
	{
		if ( json == nullptr || cJSON_IsInvalid(json) )
		{
			return false;
		}

		char* jsonString = cJSON_Print(json);

		if ( jsonString == nullptr)
		{
			return false;
		}

		if ( strlen(jsonString) == 0 )
		{
			free(jsonString);
			return false;
		}


        int bytesSend = _webSocket.sendBinaryMessage(jsonString, strlen(jsonString) );

		free(jsonString);

		if ( bytesSend == 0 )
		{
			return false;
		}

        return true;
    }

    void Connection::checkPingTimeoutWrapper(TimerHandle_t xTimer)
    {
        Connection *objectInstance = static_cast<Connection*>( pvTimerGetTimerID(xTimer) );
        objectInstance->checkPingTimeout();
    }

    void Connection::checkPingTimeout()
    {
        uint64_t difference = (getTickMs() - _lastPingTimestamp);
		ESP_LOGI(LOG_TAG, "No ping/ACK received for %f s", difference / 1000.0F);

        if ( difference > PING_TIMEOUT )
        {
            ESP_LOGW(LOG_TAG, "PING/ACK TIMEOUT: reconnect!");
            _webSocket.disconnect();
        }
    }

	void Connection::webSocketConnected()
	{
        ESP_LOGD(LOG_TAG, "webSocketConnected() running in %s", IDFix::Task::getRunningTaskName().c_str() );

        if ( _pingTimeoutTimer == nullptr )
        {
            _pingTimeoutTimer = xTimerCreate("ping_timeout", pdMS_TO_TICKS(PING_TIMEOUT_TIMER), pdTRUE, static_cast<void*>(this), &Connection::checkPingTimeoutWrapper);
            if ( _pingTimeoutTimer == nullptr )
            {
                ESP_LOGE(LOG_TAG, "Failed to create ping timeout timer");
            }
        }

        if ( xTimerStart(_pingTimeoutTimer, 0) != pdPASS )
        {
             ESP_LOGE(LOG_TAG, "Failed to start ping timeout timer");
        }

         _lastPingTimestamp = getTickMs();   // Reset ping timeout

        registerHandle();
	}

	void Connection::webSocketDisconnected()
	{
        ESP_LOGW(LOG_TAG, "webSocketDisconnected()");

        if ( _pingTimeoutTimer != nullptr )
        {
            if ( xTimerStop(_pingTimeoutTimer, 0) != pdPASS )
            {
                 ESP_LOGE(LOG_TAG, "Failed to stop ping timeout timer");
            }
        }

		if ( _eventHandler )
		{
			_eventHandler->disconnected();
		}
	}

    void Connection::webSocketBinaryMessageReceived(const char *data, int length)
	{
        std::string message = std::string(data, static_cast<unsigned long>(length) );

        ESP_LOGV(LOG_TAG, " Running in Task: %s - Connection::webSocketBinaryMessageReceived(%s)", pcTaskGetTaskName(NULL), message.c_str() );

        cJSON *jsonMessage = cJSON_Parse( message.c_str() );

		if ( jsonMessage == nullptr || ! cJSON_IsObject(jsonMessage) )
		{
            ESP_LOGE(LOG_TAG, "Invalid json message received");
			return;
		}

		// handle the message in its own function...
		handleJSONMessage(jsonMessage);

		// ... so we have to do cleanup only once
		cJSON_Delete(jsonMessage);
	}

	bool Connection::registerHandle()
	{
        ESP_LOGV(LOG_TAG, "registerHandle()");

		cJSON *registerCommand = cJSON_CreateObject();

		if (cJSON_AddStringToObject(registerCommand, "command", "connection:register") == nullptr)
		{
            ESP_LOGE(LOG_TAG, "cJSON_AddStringToObject failed");
			cJSON_Delete(registerCommand);
			return false;
		}

		if (cJSON_AddNumberToObject(registerCommand, "uuid", _connectionID) == nullptr)
		{
            ESP_LOGE(LOG_TAG, "cJSON_AddNumberToObject failed");
			cJSON_Delete(registerCommand);
			return false;
		}

		bool sendOK = sendJSON(registerCommand);
		cJSON_Delete(registerCommand);

		return sendOK;
	}

	void Connection::handleJSONMessage(const cJSON *jsonMessage)
	{
		cJSON *commandObject = cJSON_GetObjectItemCaseSensitive(jsonMessage, "command");

		if ( ! ( cJSON_IsString(commandObject) && commandObject->valuestring != nullptr ) )
		{
            ESP_LOGE(LOG_TAG, "Json message does not contain a command");
			return;
		}

		std::string command = commandObject->valuestring;

		if ( command == "ping" )
		{
            _lastPingTimestamp = getTickMs();

            cJSON *pongCommand = cJSON_CreateObject();

			if (cJSON_AddStringToObject(pongCommand, "command", "pong") == nullptr)
			{
                ESP_LOGE(LOG_TAG, "cJSON_AddStringToObject failed");
			}
			else
			{
				sendJSON(pongCommand);
			}

			cJSON_Delete(pongCommand);
			return;
		}

		if ( command == "pong" || command == "ACK" )
		{
            ESP_LOGD(LOG_TAG, "pong/ACK received");
            _lastPingTimestamp = getTickMs();

			return;
		}

		// TODO: check uuid ( if(__map.count(uuid) > 0) )

		if ( command == "connection:registered" )
		{
			_connected = true;

			if ( _eventHandler )
			{
				_eventHandler->connected();
			}

			return;
		}

		if ( command == "connection:closed" )
		{
			_connected = false;

			if ( _eventHandler )
			{
				_eventHandler->disconnected();
			}

			return;
		}

		if ( command == "send" )
		{
			cJSON *jsonPayload = cJSON_GetObjectItemCaseSensitive(jsonMessage, "payload");

			if ( jsonPayload == nullptr || ! cJSON_IsObject(jsonPayload) )
			{
                ESP_LOGE(LOG_TAG, "command:send: payload error");
				return;
			}

			if ( _eventHandler )
			{
				_eventHandler->jsonReceived(jsonPayload);
			}

			return;
		}

	}

}
