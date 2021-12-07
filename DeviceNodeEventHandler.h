#ifndef DEVICENODEEVENTHANDLER_H
#define DEVICENODEEVENTHANDLER_H

extern "C"
{
	#include <stdint.h>
}

namespace _2log
{
    /**
     * @brief The DeviceNodeEventHandler class provides an interface to handle DeviceNode events
     */
	class DeviceNodeEventHandler
	{
		public:

			virtual			~DeviceNodeEventHandler();

            /**
             * @brief This event is triggered if the DeviceNode is successfully connected to the QuickHub server
             */
			virtual void	deviceNodeConnected(void) = 0;

            /**
             * @brief This event is triggered if the DeviceNode was disconnected from the QuickHub server
             */
			virtual void	deviceNodeDisconnected(void) = 0;

            /**
             * @brief This event is triggered if the QuickHub server sent a new auth key for this device
             * @param newAuthKey    the new auth key for this device
             */
			virtual void	deviceNodeAuthKeyChanged(uint32_t newAuthKey) = 0;
	};
}

#endif
