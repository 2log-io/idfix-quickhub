#include "BaseDevice.h"

#include "Connection.h"
#include "TaskManager.h"
#include "IDFixTask.h"

#include "FirmwareUpdater.h"
#include "PublicKey.h"
#include "HashSHA256.h"
#include "ECDSASignatureVerifier.h"
#include "HTTPFirmwareDownloader.h"
#include "DeviceProperties.h"
#include <esp_wifi.h>
#include "MutexLocker.h"

using namespace IDFix;

extern "C"
{
    #include <nvs_flash.h>
}

namespace
{
    const char* LOG_TAG = "_2log::BaseDevice";
    const char* SERVER_CONFIG_PARAMETER = "server";
    const char* TEST_CONFIG_PARAMETER   = "testconfig";

    extern "C"
    {
        extern const unsigned char deviceCertificate[] asm("_binary_ipcertdummy_crt_start");
        extern const unsigned char deviceCertificateEnd[]   asm("_binary_ipcertdummy_crt_end");
        static long deviceCertificateSize = deviceCertificateEnd - deviceCertificate;

        extern const unsigned char deviceKey[] asm("_binary_ipcertdummy_key_start");
        extern const unsigned char deviceKeyEnd[]   asm("_binary_ipcertdummy_key_end");
        static long deviceKeySize = deviceKeyEnd - deviceKey;
    }


#if ALLOW_3RD_PARTY_FIRMWARE == 1

    #warning "3rd party firmware allowed"

    // Open firmware key
    unsigned char firmwareSignaturePublicKey[] =
    {
      0x30, 0x56, 0x30, 0x10, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
      0x01, 0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x0a, 0x03, 0x42, 0x00, 0x04,
      0x71, 0x91, 0xa9, 0xda, 0x8c, 0xa1, 0x0c, 0x71, 0xe2, 0x2a, 0x98, 0xc0,
      0x3e, 0x64, 0xdc, 0xf0, 0x81, 0xc9, 0xb9, 0xc8, 0x37, 0xd3, 0xee, 0xe4,
      0xa1, 0x08, 0x0b, 0x89, 0x46, 0x47, 0x11, 0x33, 0xaa, 0x11, 0x4e, 0xac,
      0xfe, 0x6e, 0xff, 0x60, 0xa3, 0xa4, 0x11, 0x1a, 0x10, 0x2f, 0x9c, 0x8c,
      0x7d, 0xfb, 0xf4, 0xc0, 0x6a, 0x48, 0x24, 0x40, 0x43, 0xeb, 0x06, 0x0b,
      0xee, 0xd3, 0x5f, 0xf9
    };

    unsigned int firmwareSignaturePublicKeyLength = 88;

#else

    // Open firmware key TODO: replace with 2log secure firmware key
    unsigned char firmwareSignaturePublicKey[] =
    {
      0x30, 0x56, 0x30, 0x10, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
      0x01, 0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x0a, 0x03, 0x42, 0x00, 0x04,
      0x71, 0x91, 0xa9, 0xda, 0x8c, 0xa1, 0x0c, 0x71, 0xe2, 0x2a, 0x98, 0xc0,
      0x3e, 0x64, 0xdc, 0xf0, 0x81, 0xc9, 0xb9, 0xc8, 0x37, 0xd3, 0xee, 0xe4,
      0xa1, 0x08, 0x0b, 0x89, 0x46, 0x47, 0x11, 0x33, 0xaa, 0x11, 0x4e, 0xac,
      0xfe, 0x6e, 0xff, 0x60, 0xa3, 0xa4, 0x11, 0x1a, 0x10, 0x2f, 0x9c, 0x8c,
      0x7d, 0xfb, 0xf4, 0xc0, 0x6a, 0x48, 0x24, 0x40, 0x43, 0xeb, 0x06, 0x0b,
      0xee, 0xd3, 0x5f, 0xf9
    };

    unsigned int firmwareSignaturePublicKeyLength = 88;

#endif

#if DISABLE_TLS_CA_VALIDATION == 1

    #warning "!!!WARNING: TLS VALIDATION DISABLED!!!11elf"

    const char* ROOT_CA = nullptr;

#else

    const char* ROOT_CA =       /* Prod CA */
		"-----BEGIN CERTIFICATE-----\n\
		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\n\
		-----END CERTIFICATE-----\n";
#endif

}

#if OVERRIDE_CONFIG == 1
    #warning "Config override enabled!"
#endif

#if DISABLE_ENERGY_METER == 1
    #warning "Energy meter disabled!"
#endif

#if OVERRIDE_WIFI == 1
    #warning "WiFi override enabled!"
#endif

#if DEVICE_DEBUGGING == 1
    #warning "Device debugging enabled!"
#endif

#if MEMORY_DEBUGGING == 1
    #warning "Memory debugging enabled!"
#endif


namespace _2log
{
    BaseDevice::BaseDevice() : IDFix::Task("update_task"), _deviceMutex(Mutex::Recursive), _settings(), _wifiManager(this)
    {
        #if SYSTEM_MONITORING == 1
            _systemMonitor.start();
        #endif

        esp_log_level_set("*", DEVICE_LOG_LEVEL );
    }

    void BaseDevice::startDevice()
    {
        esp_err_t result = nvs_flash_init();

        #ifdef CONFIG_IDF_TARGET_ESP32
            if (result == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
              ESP_ERROR_CHECK( nvs_flash_erase() );
              result = nvs_flash_init();
            }
        #endif

        if (result == ESP_ERR_NVS_NO_FREE_PAGES)
        {
            ESP_ERROR_CHECK( nvs_flash_erase() );
            result = nvs_flash_init();
        }

        ESP_ERROR_CHECK(result);

        #if ALLOW_3RD_PARTY_FIRMWARE == 1
            ESP_LOGW(LOG_TAG, "Running an unsecured firmware!");
        #else
            ESP_LOGI(LOG_TAG, "Running a secured firmware!");
        #endif

        _settings.init();
        _wifiManager.init();

        #if OVERRIDE_CONFIG == 1

            // if we set override config, skip configuration steps
            setupConnections();

        #else
            if ( _settings.isConfigured() == false)
            {
                ESP_LOGW(LOG_TAG, "Device is NOT configured");
                startConfiguration();
            }
            else
            {
                ESP_LOGI(LOG_TAG, "Device is configured");
                setupConnections();
            }
        #endif
    }

    std::string BaseDevice::getDeviceID() const
    {
        return IDFix::WiFi::WiFiManager::getStationMACAddress();
    }

    void BaseDevice::resetDeviceConfigurationAndRestart()
    {
        ESP_LOGW(LOG_TAG, "Reset device configuration and restart...");
        _settings.clearConfig();
        esp_wifi_disconnect ();
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_restart();
    }

    int8_t BaseDevice::getRSSI() const
    {
        return _wifiManager.getRSSILevel();
    }

    BaseDevice::BaseDeviceState BaseDevice::getState() const
    {
        return _deviceState;
    }

    void BaseDevice::run()
    {
        performUpdate();
    }

    void BaseDevice::startConfiguration()
    {
        _deviceState = BaseDeviceState::StartConfiguring;
        baseDeviceStateChanged(_deviceState);

        bool    configurationInitiated;
        int     retryCount = 0;

        _wifiManager.setCertificate(deviceCertificate, deviceCertificateSize);
        _wifiManager.setPrivateKey(deviceKey, deviceKeySize);
        _wifiManager.addConfigDeviceParameter("sid", _settings.getShortID() );
        _wifiManager.addConfigDeviceParameter("uuid", getDeviceID() );

        do
        {
            configurationInitiated = _wifiManager.startConfiguration(CONFIGURATION_WIFI_SSID, CONFIGURATION_WIFI_PWD);
            if ( ! configurationInitiated )
            {
                ESP_LOGE(LOG_TAG, "Failed to initiate config mode");
                IDFix::Task::delay(CONFIGURATION_RETRY_DELAY);
                retryCount++;
            }
            else
            {
                ESP_LOGI(LOG_TAG, "Initiated config mode");
            }
        }
        while ( ! configurationInitiated && retryCount < CONFIGURATION_MAX_RETRY);

        if ( ! configurationInitiated )
        {
            ESP_LOGE(LOG_TAG, "Finally failed to initiate config mode: giving up and reboot...");
            _deviceState = BaseDeviceState::Booting;
            baseDeviceStateChanged(_deviceState);
            esp_restart();
        }
    }

    void BaseDevice::configurationStarted()
    {
        _deviceState = BaseDeviceState::Configuring;
        baseDeviceStateChanged(_deviceState);
        baseDeviceEventHandler( BaseDeviceEvent::ConfigurationStarted );
        ESP_LOGI(LOG_TAG, "Started config mode waiting for config device...");
    }

    void BaseDevice::receivedWiFiConfiguration(const std::string &ssid, const std::string &password)
    {
        if ( _deviceState == BaseDeviceState::Configuring)
        {
            _settings.setWiFiSSID(ssid);
            _settings.setWiFiPassword(password);
        }
    }

    void BaseDevice::receivedConfigurationParameter(const std::string &param, const std::string &value)
    {
        if ( _deviceState == BaseDeviceState::Configuring)
        {
            ESP_LOGI(LOG_TAG, "receivedConfigurationParameter( %s, %s )", param.c_str(), value.c_str() );

            if ( param == SERVER_CONFIG_PARAMETER )
            {
                _settings.setServerURL(value);
            }
        }
    }

    void BaseDevice::receivedConfigurationParameter(const std::string &param, const bool value)
    {
        if ( _deviceState == BaseDeviceState::Configuring)
        {
            ESP_LOGI(LOG_TAG, "receivedConfigurationParameter( %s, %d )", param.c_str(), value );

            if ( param == TEST_CONFIG_PARAMETER )
            {
                _testReceivedWifiConfig = value;
            }
        }
    }

    void BaseDevice::configurationFinished()
    {
        ESP_LOGI(LOG_TAG, "Configuration Finished: Saving config and rebooting to test config...");
        _settings.saveConfiguration();
        _deviceState = BaseDeviceState::Booting;
        baseDeviceStateChanged(_deviceState);
        baseDeviceEventHandler(BaseDeviceEvent::ConfigurationSucceeded);

        DeviceProperties::instance().saveProperty(".configReset", _testReceivedWifiConfig);

        #if ALLOW_3RD_PARTY_FIRMWARE == 1

            ESP_LOGW(LOG_TAG, "First 2log configuration on an open firmware! Switching to secured firmware!");
            IDFix::FOTA::FirmwareUpdater::activateNextUpdatePartition();

        #endif

        esp_restart();
    }

    void BaseDevice::networkDisconnected()
    {
        _deviceMutex.lock();
            _networkConnected = false;
        _deviceMutex.unlock();

        ESP_LOGW(LOG_TAG, "WiFi disconnected! (running in task %s)", Task::getRunningTaskName().c_str() );
        bool success;

        if( DeviceProperties::instance().getProperty(".configReset", false).asBool(&success) && success)
        {
            // this was the first start after a configuration and wifi connection was not successful
            // we asume the configuration was incorrect, clear the configuration and restart in configuration mode again

            resetDeviceConfigurationAndRestart();
        }
        else
        {
            connectWiFi();
        }
    }

    void BaseDevice::networkConnected(const IDFix::WiFi::IPInfo &ipInfo)
    {
        DeviceProperties::instance().saveProperty(".configReset", false);

        _deviceMutex.lock();
            _networkConnected = true;
            _deviceNodeConnectionRetries = 0;
        _deviceMutex.unlock();

        ESP_LOGI(LOG_TAG, "networkConnected - IP: " IPSTR, IP2STR(&ipInfo.ip) );

        baseDeviceEventHandler(BaseDeviceEvent::NetworkConnected);

        _deviceNode->connect();
    }

    void BaseDevice::deviceNodeConnected()
    {
        ESP_LOGI(LOG_TAG, "deviceNodeConnected");

        _deviceMutex.lock();
            _deviceNodeConnectionRetries = 0;
        _deviceMutex.unlock();

        baseDeviceEventHandler(BaseDeviceEvent::NodeConnected);

        _deviceMutex.lock();
            _deviceState = BaseDeviceState::Connected;
        _deviceMutex.unlock();

        baseDeviceStateChanged(_deviceState);
    }

    void BaseDevice::deviceNodeDisconnected()
    {
        ESP_LOGW(LOG_TAG, "DeviceNode disconnected (running in task %s)", Task::getRunningTaskName().c_str() );

        uint32_t delayTime = 0;

        MutexLocker locker(_deviceMutex);

        if ( _networkConnected )
        {
            baseDeviceEventHandler(BaseDeviceEvent::NodeDisconnected);

            _deviceNodeConnectionRetries++;

            if ( _deviceNodeConnectionRetries > CONNECTION_RETRY_LIMIT_UNTIL_WIFI_RECONNECT )
            {
                ESP_LOGE(LOG_TAG, "Device node reconnection limit (WiFi) reached!");
                //ESP_LOGE(LOG_TAG, "Device node reconnection limit (WiFi) reached! Restart WiFi...");

                // TODO: Implement disconnect in WiFiManager
                // TODO: disconnect and reconnect WiFi
            }

            if ( _deviceNodeConnectionRetries > CONNECTION_RETRY_LIMIT_UNTIL_DELAY )
            {
                delayTime = CONNECTION_RETRY_DELAY_TIME;
            }

            ESP_LOGI(LOG_TAG, "Trying to reconnect deviceNode...");

            _deviceState = BaseDeviceState::Connecting;

            locker.unlock();

            baseDeviceStateChanged(_deviceState);

            _deviceNode->connect(delayTime);
        }
    }

    void BaseDevice::deviceNodeAuthKeyChanged(uint32_t newAuthKey)
    {
        _settings.writeAuthKey(newAuthKey);
    }

    void BaseDevice::initProperties(cJSON *argument)
    {
        cJSON_AddStringToObject(argument, ".ip",         WiFi::IPInfo::ipToString( getIPInfo().ip ).c_str() );
        cJSON_AddStringToObject(argument, ".mac",        WiFi::WiFi::getStationMACAddress().c_str() );
        cJSON_AddNumberToObject(argument, ".rssi",       getRSSI());
        cJSON_AddNumberToObject(argument, ".fwstatus",   static_cast<int>(FirmwareState::Stable) );
    }

    bool BaseDevice::connectWiFi()
    {
        baseDeviceEventHandler(BaseDeviceEvent::NetworkConnecting);

        _deviceState = BaseDeviceState::Connecting;
        baseDeviceStateChanged(_deviceState);

        #if OVERRIDE_WIFI == 1
            ESP_LOGI(LOG_TAG, "Connection to ssid: %s", WIFI_SSID);
            return _wifiManager.connectWPA(WIFI_SSID, WIFI_PASSWORD);
        #else
            ESP_LOGI(LOG_TAG, "Connecting to ssid: %s", _settings.getWiFiSSID().c_str() );
            return _wifiManager.connectWPA(_settings.getWiFiSSID(), _settings.getWiFiPassword() );
        #endif
    }

    void BaseDevice:: setupConnections()
    {
        ESP_LOGI(LOG_TAG, "Device configured and starts running");

        std::string connectionURL;

        #if OVERRIDE_CONFIG == 1
            connectionURL = SERVER_URL;
        #else
            connectionURL = _settings.getServerURL();
        #endif

        _deviceNode = new DeviceNode( new Connection( connectionURL, ROOT_CA ), this, DEVICE_TYPE, getDeviceID(), _settings.getShortID(), _settings.getAuthKey() );

        _deviceNode->registerInitPropertiesCallback(std::bind(&BaseDevice::initProperties, this, std::placeholders::_1) );
        _deviceNode->registerRPC(".fwupdate",       std::bind(&BaseDevice::updateFirmwareRPC, this, std::placeholders::_1) );

        connectWiFi();

        #if DUMP_TASK_STATS == 1
            while (true)
            {
                IDFix::Task::delay(5000);
                IDFix::TaskManager::printTaskList();
            }
        #endif
    }

    void BaseDevice::updateFirmwareRPC(cJSON *argument)
    {
        ESP_LOGI(LOG_TAG, "Firmware update triggered (running in task %s)", Task::getRunningTaskName().c_str() );

        if(_deviceState == BaseDeviceState::UpdatingFirmware)
        {
            ESP_LOGI(LOG_TAG, "Update already in progress..");
            return;
        }

        cJSON *urlValItem = cJSON_GetObjectItem(argument, "val");
        if ( cJSON_IsString(urlValItem) )
        {
            _stateBeforeFirmwareUpdate = _deviceState;
            baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateStarted);
            _deviceState = BaseDeviceState::UpdatingFirmware;
            baseDeviceStateChanged(_deviceState);

            ESP_LOGI(LOG_TAG, "Update URL: %s", urlValItem->valuestring);
            _deviceNode->setProperty(".fwstatus", static_cast<int>(FirmwareState::InitUpdate) );
            _updateURL = urlValItem->valuestring;

            startTask();
        }
        else
        {
            ESP_LOGE(LOG_TAG, "cJSON_IsString(urlValItem) failed");
            _deviceNode->setProperty(".fwstatus", static_cast<int>(FirmwareState::InvalidUpdateArgument) );
        }
    }

    void BaseDevice::performUpdate()
    {
        ESP_LOGI(LOG_TAG, "Performing firmware update from URL %s", _updateURL.c_str() );

        FOTA::FirmwareUpdater           updater;
        FOTA::HTTPFirmwareDownloader    downloader;
        Crypto::PublicKey               publicKey;
        Crypto::HashSHA256              hashSHA256;
        Crypto::ECDSASignatureVerifier  signatureVerifier;

        int result;

        if ( (result = publicKey.parseKey(firmwareSignaturePublicKey, firmwareSignaturePublicKeyLength) ) != 0 )
        {
            ESP_LOGE(LOG_TAG, "Failed to parse public key!");
            baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateFailed);
            _deviceState = _stateBeforeFirmwareUpdate;
            baseDeviceStateChanged(_deviceState);
            return;
        }

        if ( signatureVerifier.setPublicKey(&publicKey) != 0 )
        {
            ESP_LOGE(LOG_TAG, "Failed to set public key for verifier!");
            baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateFailed);
            _deviceState = _stateBeforeFirmwareUpdate;
            baseDeviceStateChanged(_deviceState);
            return;
        }

        updater.installSignatureVerifier(&signatureVerifier, &hashSHA256);

        if ( ! updater.setMagicBytes(FIRMWARE_MAGIC_BYTES, strlen(FIRMWARE_MAGIC_BYTES)) )
        {
            ESP_LOGE(LOG_TAG, "Failed to setMagicBytes. Aborting...");
            baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateFailed);
            _deviceState = _stateBeforeFirmwareUpdate;
            baseDeviceStateChanged(_deviceState);
            return;
        }

        downloader.setFirmwareWriter( &updater );

        esp_http_client_config_t httpConfig;
        memset(&httpConfig, 0, sizeof(esp_http_client_config_t) );
        httpConfig.url = _updateURL.c_str();

        if ( updater.beginUpdate() )
        {
            _deviceNode->setProperty(".fwstatus", static_cast<int>(FirmwareState::UpdateDownloading) );

            if ( downloader.downloadFirmware(&httpConfig) == 0 )
            {
                if ( updater.finishUpdate() )
                {
                    ESP_LOGI(LOG_TAG, "Firmware update successful. Restarting...");
                    _deviceNode->setProperty(".fwstatus", static_cast<int>(FirmwareState::UpdateSucceeded) );
                    baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateSucceeded);
                    Task::delay(3000);
                    esp_restart();
                }
                else
                {
                    ESP_LOGE(LOG_TAG, "Failed to finish firmware update...");
                    _deviceNode->setProperty(".fwstatus", static_cast<int>(FirmwareState::UpdateFailed) );
                    baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateFailed);
                    _deviceState = _stateBeforeFirmwareUpdate;
                    baseDeviceStateChanged(_deviceState);
                    Task::delay(3000);
                    esp_restart();
                }
            }
            else
            {
                ESP_LOGE(LOG_TAG, "Failed to download firmware...");
                _deviceNode->setProperty(".fwstatus", static_cast<int>(FirmwareState::UpdateDownloadFailed) );
                baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateFailed);
                _deviceState = _stateBeforeFirmwareUpdate;
                baseDeviceStateChanged(_deviceState);
                updater.abortUpdate();
            }
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Failed to start update...");
            _deviceNode->setProperty(".fwstatus", static_cast<int>(FirmwareState::UpdateStartFailed) );
            baseDeviceEventHandler(BaseDeviceEvent::FirmwareUpdateFailed);
            _deviceState = _stateBeforeFirmwareUpdate;
            baseDeviceStateChanged(_deviceState);
        }
    }

    void BaseDevice::baseDeviceEventHandler(BaseDeviceEvent event)
    {
        ESP_LOGD(LOG_TAG, "Default baseDeviceEventHandler triggered");
    }

    void BaseDevice::baseDeviceStateChanged(BaseDeviceState state)
    {
        ESP_LOGD(LOG_TAG, "Default baseDeviceStateChanged handler triggered");
    }

    IDFix::WiFi::IPInfo _2log::BaseDevice::getIPInfo() const
    {
        return _wifiManager.getStationIPInfo();
    }

}
