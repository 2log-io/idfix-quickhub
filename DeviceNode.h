#ifndef DEVICENODE_H
#define DEVICENODE_H

#include <functional>
#include <map>
#include "StringComparison.h"
#include "ConnectionEventHandler.h"
#include "IConnection.h"
#include "IDeviceNode.h"
#include "DeviceSettings.h"

struct cJSON;

namespace _2log
{
	class DeviceNodeEventHandler;

    /**
     * @brief The DeviceNode class represents a device on the QuickHub instance
     */
	class DeviceNode : public IDeviceNode, public ConnectionEventHandler
	{
		public:

            /**
             * @brief Constructs a new DeviceNode
             * @param connection    the connection to the QuickHub instance
             * @param eventHandler  the event handler for this node
             * @param nodeType      the type of this node as string
             * @param id            the unique ID of this device
             * @param shortID       the short ID of this device
             * @param authKey       the authentication key of this device
             */
							DeviceNode(IConnection *connection, DeviceNodeEventHandler *eventHandler, const std::string &nodeType,
									   const std::string &id, const std::string &shortID, const uint32_t authKey);

			virtual			~DeviceNode() override;

            /**
             * @brief Set the event handler for this device node.
             * @return \c true if event handler was successfully set, \c false otherwise
             */
			virtual bool	setDeviceNodeEventHandler(DeviceNodeEventHandler*) override;

            /**
             * @brief Attempts to connect to the server
             * @param delayTime     an optional time to delay the connection attempt in milliseconds
             *
             * @return      true if connection attempt was queued
             * @return      false if connection attempt could not be queued - connect must be called again
             */
			virtual bool	connect(uint32_t delayTime = 0) override;

            /**
             * @brief Attempts to disconnect from the server
             * @return      true if disconnect attempt was queued
             * @return      false if disconnect attempt could not be queued - disconnect must be called again
             */
			virtual bool	disconnect(void) override;

            /**
             * @brief Register a callback function to initially set the properties of the device
             * @param callbackFunction  the callback to call upon the initialization
             */
			virtual void	registerInitPropertiesCallback(jsonCallbackFunction callbackFunction) override;

            /**
             * @brief Register a RPC callback for this device
             * @param name      the RPC name
             * @param callback  the RPC callback function
             */
			virtual void	registerRPC(const char *name, jsonCallbackFunction callback) override;

            /**
             * @brief Send generic data to the QuickHub server
             * @param subject   the data message to send
             * @return  \c true if data was sent successfully, \c false otherwise
             */
			virtual bool	sendData(const char *subject) override;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
			virtual void	setProperty(const char* property, int value) override;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
            virtual void	setProperty(const char* property, float value) override;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
            virtual void    setProperty(const char *property, const char* value) override;

            /**
             * @brief Send a changed property value to the QuickHub server
             * @param property  the property name
             * @param value     the property value
             */
            virtual void    setProperty(const char *property, bool value) override;

            /**
             * @brief Send changed propertie values to the QuickHub server
             * @param parameters    the changed properties as cJSON object
             */
			void			setProperties(cJSON *parameters);

            /**
             * @brief Handles the connection connected event
             */
			virtual void	connected(void) override;

            /**
             * @brief Handles the connection disconnected event
             */
			virtual void	disconnected(void) override;

            /**
             * @brief Handles a received JSON message
             */
			virtual void	jsonReceived(const cJSON*) override;

		private:

            /**
             * @brief Call a RPC with the specified argument
             * @param name      the RPC name
             * @param argument  the RPC argument
             */
			void			callRPC(const char *name, cJSON* argument);

            /**
             * @brief Registers the DeviceNode with the QuickHub server
             * @param registerObject    the cJSON register object
             */
			void			registerNode(cJSON *registerObject);

		private:

			IConnection*													_connection;
            bool                                                            _isConnected = { false };

			const std::string												_nodeType;
			const std::string												_id;
			const std::string												_shortID;
			const uint32_t													_authKey;
			DeviceNodeEventHandler*											_eventHandler = { nullptr };

			jsonCallbackFunction											_initPropertiesCallback = {};
			std::map<const char*, jsonCallbackFunction, StringComparison>	_rpcCallbacks = {};
	};
}

#endif
