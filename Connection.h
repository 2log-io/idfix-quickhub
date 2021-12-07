#ifndef CONNECTION_H
#define CONNECTION_H

#include "WebSocketEventHandler.h"
#include "WebSocket.h"
#include "ConnectionEventHandler.h"

#include "IConnection.h"
#include <cJSON.h>
#include <string>

extern "C"
{
    #include <freertos/timers.h>
}

namespace _2log
{
    /**
     * @brief The Connection class represents a connection to a QuickHub instance.
     *
     * This class basically is the device equivalent of the QuickHub Connection class. It encapsulates a WebSocket
     * connection and draws up the first layer of the QuickHub JSON protocol. Unlike the server equivalent this class
     * currently does not support multiple sub-connections and implicitly establishes a VirtualConnection to the
     * QuickHub instance. So this class currently combines the server side Connection and VirtualConnection class.
     */
    class Connection : public IDFix::Protocols::WebSocketEventHandler, public IConnection
	{
		public:

            /**
             * @brief Constructs a new Connection
             *
             * @param url               the websocket URL to the server
             * @param caCertificate     optional the root CA X.509 certificate in PEM format as null-terminated string
             */
										Connection(const std::string &url, const char *caCertificate = nullptr);

            /**
             * @brief Attempts to connect to the server
             * @param delayTime     an optional time to delay the connection attempt in milliseconds
             *
             * @return      true if connection attempt was queued
             * @return      false if connection attempt could not be queued - connect must be called again
             */
			virtual bool				connect(uint32_t delayTime = 0) override;

            /**
             * @brief Attempts to disconnect from the server
             * @return      true if disconnect attempt was queued
             * @return      false if disconnect attempt could not be queued - disconnect must be called again
             */
			virtual bool				disconnect(void) override;

            /**
             * @brief Send a JSON payload to the server
             * @return  \c true on success, \c false otherwise
             */
			virtual bool				sendPayload(const cJSON*) override;

            /**
             * @brief Sets the event handler for this connection
             * @param handler   pointer to the event handler
             * @return  \c true on success, \c false otherwise
             */
			virtual bool				setConnectionEventHandler(ConnectionEventHandler* newEventHandler) override;

            /**
             * @brief Handles the websocket connected event
             */
			virtual void				webSocketConnected(void) override;

            /**
             * @brief Handles the websocket disconnected event
             */
			virtual void				webSocketDisconnected(void) override;

            /**
             * @brief Handle received websocket message
             * @param data      the received binary message
             * @param length    the message length in bytes
             */
            virtual void				webSocketBinaryMessageReceived(const char* data, int length) override;

		private:

            /**
             * @brief Register a new virtual connection on the QuickHub server
             * @return \c true if register request was sent, \c false otherwise
             */
			bool						registerHandle(void);

            /**
             * @brief Handle a JSON message from the QuickHub server
             * @param jsonMessage   the message as cJSON object
             */
			void						handleJSONMessage(const cJSON *jsonMessage);

            /**
             * @brief Convert the JSON onbject to a string and send it to the server.
             * @return  \c true if messages was successfully sent, \c false otherwise
             */
			bool						sendJSON(const cJSON*);

            /**
             * @brief Static timer wrapper function
             * @param xTimer    the FreeRTOS timer handle
             */
            static void                 checkPingTimeoutWrapper(TimerHandle_t xTimer);

            /**
             * @brief Timer callback to check if connection is still alive
             */
            void                        checkPingTimeout(void);

		private:

			std::string					_serverURL;
			uint8_t						_connectionID;
			bool						_connected;
            IDFix::Protocols::WebSocket _webSocket;
			ConnectionEventHandler*		_eventHandler;
            uint64_t                    _lastPingTimestamp = { 0 };
            TimerHandle_t               _pingTimeoutTimer = { nullptr };
	};
}

#endif
