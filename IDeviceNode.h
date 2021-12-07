#ifndef IDEVICENODE_H
#define IDEVICENODE_H

#include <functional>

// Forward declaration
class cJSON;

namespace _2log
{
	class DeviceNodeEventHandler;

    /**
     * @brief The IDeviceNode class provides an interface to a QuickHub DeviceNode
     */
	class IDeviceNode
	{
		public:

			typedef			std::function<void(cJSON*)>	jsonCallbackFunction;

			virtual			~IDeviceNode();

            /**
             * @brief Set the event handler for this device node.
             * @return \c true if event handler was successfully set, \c false otherwise
             */
			virtual bool	setDeviceNodeEventHandler(DeviceNodeEventHandler*) = 0;

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
             * @brief Register a callback function to initially set the properties of the device
             * @param callbackFunction  the callback to call upon the initialization
             */
			virtual void	registerInitPropertiesCallback(jsonCallbackFunction callbackFunction) = 0;

            /**
             * @brief Register a RPC callback for this device
             * @param name      the RPC name
             * @param callback  the RPC callback function
             */
			virtual void	registerRPC(const char *name, jsonCallbackFunction callback) = 0;

            /**
             * @brief Send generic data to the QuickHub server
             * @param subject   the data message to send
             * @return  \c true if data was sent successfully, \c false otherwise
             */
			virtual bool	sendData(const char *subject) = 0;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
			virtual void	setProperty(const char* property, int value) = 0;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
            virtual void	setProperty(const char* property, float value) = 0;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
            virtual void	setProperty(const char* property, const char* value) = 0;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
            virtual void    setProperty(const char *property, bool value) = 0;
	};
}


#endif
