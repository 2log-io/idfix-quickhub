#ifndef BASEDEVICE_H
#define BASEDEVICE_H

#include "BuildConfig.h"

#include "DeviceNode.h"
#include "DeviceSettings.h"
#include "DeviceNodeEventHandler.h"
#include "WiFiManagerEventHandler.h"
#include "WiFiManager.h"
#include "driver/gpio.h"
#include "IDFixTask.h"
#include "Mutex.h"

#if SYSTEM_MONITORING == 1
    #include "SystemMonitor.h"
#endif

namespace _2log
{
    /**
     * @brief The BaseDevice class provides a base implementation for a QuickHub device.
     *
     * BaseDevice handles the configuration and the start of the device and automatically connects
     * to the configured WiFi network and the configured QuickHub server.
     */
    class BaseDevice :
            public DeviceNodeEventHandler,
            public IDFix::WiFi::WiFiManagerEventHandler,
            private IDFix::Task
    {
        public:

            enum class BaseDeviceState
            {
                Booting,
                StartConfiguring,
                Configuring,
                Connecting,
                Connected,
                UpdatingFirmware
            };

                                BaseDevice(void);

            /**
             * @brief Initialize and start the device
             *
             * If the device is not configured the configuration mode will be started. Otherwise
             * the device connects to the configured WiFi network and server.
             */
            void                startDevice(void);

            /**
             * @brief Get the unique device ID
             * @return  the ID of the device
             */
            std::string         getDeviceID(void) const;

            /**
             * @brief Get the current RSSI level of the connected WIFI
             * @return the current RSSI level
             */
            int8_t              getRSSI(void) const;

            /**
             * @brief Get the current IP addresses of the device
             * @return the current IP addresses
             */
            IDFix::WiFi::IPInfo getIPInfo() const;

            /**
             * @brief Get the current state of the device
             * @return  the current state
             */
            BaseDeviceState     getState() const;

            enum class BaseDeviceEvent
            {
                Undefined,
                ConfigurationStarted,
                ConfigurationFailed,
                ConfigurationSucceeded,
                NetworkConnecting,
                NetworkConnected,
                NetworkDisconnected,
                NodeConnected,
                NodeDisconnected,
                FirmwareUpdateStarted,
                FirmwareUpdateFailed,
                FirmwareUpdateSucceeded
            };

            enum class FirmwareState
            {
                UpdateFailed            = -4,
                UpdateDownloadFailed    = -3,
                UpdateStartFailed       = -2,
                InvalidUpdateArgument   = -1,
                Stable                  = 0,
                InitUpdate              = 1,
                UpdateDownloading       = 2,
                UpdateSucceeded         = 3,
                Testing                 = 4 // reserved for future use
            };

        protected:  // functions

            virtual void        run() override;

            /**
             * @brief Init the DeviceNode properties for this device
             * @param the DeviceNode properties object
             */
            virtual void        initProperties(cJSON* argument);

            /**
             * @brief Clear the device configuration and restart the device
             */
            virtual void        resetDeviceConfigurationAndRestart();

        private:    // functions

            /**
             * @brief Start the device configuration mode.
             */
            void                startConfiguration(void);

            /**
             * @brief Handle the configurationStarted event from the WiFiManager and initiate device state changes.
             */
            virtual void        configurationStarted() override;

            /**
             * @brief Process the received WiFi configuration.
             * @param ssid          the SSID for the configured WiFi
             * @param password      the password for the configured WiFi
             */
            virtual void        receivedWiFiConfiguration(const std::string &ssid, const std::string &password) override;

            virtual void        receivedConfigurationParameter(const std::string &param, const std::string &value) override;
            virtual void        receivedConfigurationParameter(const std::string &param, const bool value) override;

            virtual void        configurationFinished() override;

            virtual void        networkDisconnected(void) override;
            virtual void        networkConnected(const IDFix::WiFi::IPInfo &ipInfo) override;

            virtual void        deviceNodeConnected(void) override;
            virtual void        deviceNodeDisconnected(void) override;

            virtual void        deviceNodeAuthKeyChanged(uint32_t newAuthKey) override;

            bool                connectWiFi(void);
            void                setupConnections(void);

            void                updateFirmwareRPC(cJSON* argument);
            void                performUpdate(void);

            // new event and state handlers for subclassed device implementations
            virtual void        baseDeviceEventHandler(BaseDeviceEvent event);
            virtual void        baseDeviceStateChanged(BaseDeviceState state);

        protected:  // attributes

            DeviceNode*                     _deviceNode = { nullptr };
            IDFix::Mutex                    _deviceMutex;

        private:    // attributes

            BaseDeviceState                 _deviceState = { BaseDeviceState::Booting };
            BaseDeviceState                 _stateBeforeFirmwareUpdate;
            DeviceSettings                  _settings;
            IDFix::WiFi::WiFiManager        _wifiManager;
            bool                            _networkConnected = { false };
            bool                            _testReceivedWifiConfig = { true };
            int                             _deviceNodeConnectionRetries = { 0 };
            gpio_num_t                      _resetPin;
            std::string                     _updateURL;

            #if SYSTEM_MONITORING == 1
                IDFix::SystemMonitor        _systemMonitor;
            #endif
    };
}

#endif
