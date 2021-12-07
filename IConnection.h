#ifndef ICONNECTION_H
#define ICONNECTION_H

#include <stdint.h>

// Forward declaration
class cJSON;

namespace _2log
{
	class ConnectionEventHandler;

    /**
     * @brief The IConnection class provides an interface to a QuickHub connection
     */
	class IConnection
	{
		public:

			virtual			~IConnection();

            /**
             * @brief Attempts to connect to the server
             * @param delayTime     an optional time to delay the connection attempt in milliseconds
             *
             * @return      true if connection attempt was queued
             * @return      false if connection attempt could not be queued - connect must be called again
             */
			virtual bool	connect(uint32_t delayTime = 0) = 0;

            /**
             * @brief Attempts to disconnect from the server
             * @return      true if disconnect attempt was queued
             * @return      false if disconnect attempt could not be queued - disconnect must be called again
             */
			virtual bool	disconnect(void) = 0;

            /**
             * @brief Send a JSON payload to the server
             * @return  \c true on success, \c false otherwise
             */
			virtual bool	sendPayload(const cJSON*) = 0;

            /**
             * @brief Sets the event handler for this connection
             * @param handler   pointer to the event handler
             * @return  \c true on success, \c false otherwise
             */
			virtual bool	setConnectionEventHandler(ConnectionEventHandler*) = 0;
	};
}


#endif
