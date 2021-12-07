#ifndef CONNECTIONEVENTHANDLER_H
#define CONNECTIONEVENTHANDLER_H

#include <cJSON.h>

namespace _2log
{
    /**
     * @brief The ConnectionEventHandler class provides an interface to handle connection events
     */
	class ConnectionEventHandler
	{
		public:

			virtual			~ConnectionEventHandler();

            /**
             * @brief This event is triggered if the connection to the QuickHub server is successfully established
             */
			virtual void	connected(void) = 0;

            /**
             * @brief This event is triggered if the connection to the QuickHub server was disconnected
             */
			virtual void	disconnected(void) = 0;

            /**
             * @brief This event is triggered when a JSON payload was received from the QuickHub server
             */
			virtual void	jsonReceived(const cJSON*) = 0;
	};
}

#endif
