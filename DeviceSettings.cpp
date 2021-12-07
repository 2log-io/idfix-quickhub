#include "DeviceSettings.h"

#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "WMath.h"
#include <cJSON.h>
#include <stdlib.h>

#include "BuildConfig.h"

extern "C"
{
    #include "mbedtls/aes.h"
    #include "mbedtls/md5.h"
}

namespace
{
	const char* LOG_TAG                 = "_2log::DeviceSettings";
	const char* AUTHKEY_FILE            = "/2log/auth.json";
	const char* DEVICE_SETTINGS_FILE    = "/2log/settings.json";
	const char* DEVICE_ID_FILE          = "/2log/device-id.txt";

	// just for obfuscation
	const char* SSID_SETTINGS_NAME      = "sensor-calibration";
	const char* PASSWORD_SETTINGS_NAME  = "calibration-data";
}

namespace _2log
{
    DeviceSettings::DeviceSettings():
    _deviceShortID("")
	{

	}

	bool DeviceSettings::init()
	{
        DataStorage::getInstance().mount("/2log");

		loadDeviceShortID();
		loadAuthKey();
		loadDeviceSettings();

		return true;
	}

	bool DeviceSettings::isConfigured() const
	{
		// if any of the config parameters is empty the device is not fully configured
		if ( _wifiSSID.empty() || _wifiPassword.empty() || _serverURL.empty() )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	bool DeviceSettings::clearConfig()
	{
		_wifiSSID.clear();
		_wifiPassword.clear();
		_serverURL.clear();

        if ( DataStorage::getInstance().deleteFile(DEVICE_SETTINGS_FILE) == false )
		{
			// file could not be deleted
			ESP_LOGE(LOG_TAG, "could not delete file %s", DEVICE_SETTINGS_FILE);
			return false;
		}

		return true;
	}

	const char *DeviceSettings::getShortID() const
	{
		return _deviceShortID;
	}

	uint32_t DeviceSettings::getAuthKey() const
	{
		return _authKey;
	}

	void DeviceSettings::loadAuthKey()
	{
        const char *authJsonString = DataStorage::getInstance().readTextFile(AUTHKEY_FILE);

		cJSON *authJson = cJSON_Parse(authJsonString);

		if ( cJSON_IsObject(authJson) )
		{
			cJSON *keyItem = cJSON_GetObjectItem(authJson, "key");
			if ( cJSON_IsNumber(keyItem) )
			{
				_authKey = static_cast<uint32_t>(keyItem->valueint);
				ESP_LOGD(LOG_TAG, "_authKey loaded: %u", _authKey);
			}
			else
			{
				ESP_LOGE(LOG_TAG, "cJSON_IsNumber(keyItem) failed");
			}
		}
		else
		{
			ESP_LOGE(LOG_TAG, "cJSON_IsObject(authJson) failed");
		}

		cJSON_free(authJson);
        delete authJsonString;
    }

    void DeviceSettings::getEncryptionPass(unsigned char encryptionPass[])
    {
        uint8_t		wifiMACAddress[MAC_ADDR_LEN] = {};

        esp_read_mac(wifiMACAddress, ESP_MAC_WIFI_STA);

        // adding an additional challenge for the reverse engineer ;) try harder...
        uint8_t temp;
        temp = wifiMACAddress[5];
        wifiMACAddress[5] = wifiMACAddress[0];
        wifiMACAddress[0] = temp;

        char    wifiMACString[MAC_STR_LEN + 1];
        snprintf(wifiMACString, MAC_STR_LEN + 1, "%02X:%02X:%02X:%02X:%02X:%02X", wifiMACAddress[0], wifiMACAddress[1], wifiMACAddress[3], wifiMACAddress[2], wifiMACAddress[4], wifiMACAddress[5]);

        unsigned char* passPointer = encryptionPass;

        uint16_t salt = 0x2106;
        volatile int saltLen = 2;

        memcpy(passPointer, &salt, sizeof(salt) );
        passPointer += sizeof(salt);
        memcpy(passPointer, wifiMACString, MAC_STR_LEN );

        // flash_id hat nix mit dem encrpytion pass zu tun - das "ist nur um die Russen zu verwirren" ;)
        const char *foo = "Failed to read flash_id...";

        // made to look like some kind of error check and to include a fake debugging message
        if ( passPointer == NULL)
        {
            memcpy(wifiMACString, foo, MAC_STR_LEN);
        }

        passPointer += MAC_STR_LEN;
        memcpy(passPointer, &salt, sizeof(salt) );

        const char *foo2 = "Failed to generate SHA1...";
        if ( passPointer == NULL )
        {
            memcpy(wifiMACString, foo2, MAC_STR_LEN);
        }

        encryptionPass[sizeof(salt) + MAC_STR_LEN + saltLen] = 0x00;
    }

    uint8_t DeviceSettings::encrypt(const char *input, size_t inputLength, const unsigned char *encryptionPass, unsigned char ciphertext[])
    {
        if ( inputLength > MAX_ENCRYPTION_LEN )
        {
            return 0;
        }

        unsigned char 	encryptionKey[16];
        mbedtls_md5_ret(encryptionPass, ENCRYPTION_PASS_LEN, encryptionKey);

        const int blockSize = 16;
        int paddedLength = inputLength + ( ( blockSize - ( inputLength % blockSize ) ) % blockSize );

        unsigned char inputPadded[64] = {0};
        memcpy(inputPadded, input, inputLength);


        unsigned char iv[16] = { 0x21, 0x06, 0xAC, 0xDC, 0xBA, 0xDA, 0x55, 0x00, 0x00, 0x21, 0x06, 0xAC, 0xDC, 0xBA, 0xDA, 0x55 };

        memset(ciphertext, 0, MAX_ENCRYPTION_LEN);

        mbedtls_aes_context aesContext;

        mbedtls_aes_init( &aesContext );
        mbedtls_aes_setkey_enc( &aesContext, encryptionKey, sizeof(encryptionKey) * 8);
        mbedtls_aes_crypt_cbc( &aesContext, MBEDTLS_AES_ENCRYPT, paddedLength, iv, inputPadded, ciphertext );
        mbedtls_aes_free( &aesContext );

        return paddedLength;
    }

    void DeviceSettings::decrypt(const unsigned char *ciphertext, uint8_t ciphertextLen, const unsigned char *decryptionPass, unsigned char cleartext[])
    {
        if ( ciphertextLen > MAX_ENCRYPTION_LEN )
        {
            memset(cleartext, 0, MAX_ENCRYPTION_LEN);
            return;
        }

        unsigned char 	decryptionKey[16];
        mbedtls_md5_ret(decryptionPass, ENCRYPTION_PASS_LEN, decryptionKey);

        unsigned char iv[16] = { 0x21, 0x06, 0xAC, 0xDC, 0xBA, 0xDA, 0x55, 0x00, 0x00, 0x21, 0x06, 0xAC, 0xDC, 0xBA, 0xDA, 0x55 };

        memset(cleartext, 0, MAX_ENCRYPTION_LEN);

        mbedtls_aes_context aesContext;

        mbedtls_aes_init( &aesContext );
        mbedtls_aes_setkey_dec( &aesContext, decryptionKey, sizeof(decryptionKey) * 8);
        mbedtls_aes_crypt_cbc( &aesContext, MBEDTLS_AES_DECRYPT, ciphertextLen, iv, ciphertext, cleartext );
        mbedtls_aes_free( &aesContext );
    }

    void DeviceSettings::decryptionToStdString(const char *ciphertextHexString, const unsigned char *decryptionPass, std::string &output)
    {
        output = "";

        unsigned char   ciphertext[MAX_ENCRYPTION_LEN];
        unsigned char   cleartext[MAX_ENCRYPTION_LEN];

        uint8_t hexStringLength = strlen(ciphertextHexString);

        if ( (hexStringLength % 2) != 0 )
        {
            return;
        }

        uint8_t ciphertextLength = hexStringLength / 2;

        if ( ciphertextLength > MAX_ENCRYPTION_LEN )
        {
            return;
        }

        for (uint8_t i = 0, j = 0; j < ciphertextLength; i+=2, j++)
        {
            ciphertext[j] = (ciphertextHexString[i] % 32 + 9) % 25 * 16 + (ciphertextHexString[i+1] % 32 + 9) % 25;
        }

        decrypt(ciphertext, ciphertextLength, decryptionPass, cleartext);

        output = std::string(reinterpret_cast<const char*>(cleartext), strlen( reinterpret_cast<char*>(cleartext) ) );
    }

	void DeviceSettings::writeAuthKey(uint32_t authKey)
	{
		_authKey = authKey;

		cJSON *jsonConfig = cJSON_CreateObject();

		if (cJSON_AddNumberToObject(jsonConfig, "key", authKey) == nullptr)
		{
			ESP_LOGE(LOG_TAG, "cJSON_AddNumberToObject failed - authKey");
			cJSON_Delete(jsonConfig);
			return;
		}

		char *jsonString = cJSON_Print(jsonConfig);

        if ( DataStorage::getInstance().writeTextFile(AUTHKEY_FILE, jsonString) > 0)
		{
			ESP_LOGD(LOG_TAG, "_authKey written: %u", _authKey);
		}

		// cJSON uses malloc(), don't mix malloc/new + free/delete
        free(jsonString);
		cJSON_Delete(jsonConfig);
	}

	const std::string &DeviceSettings::getWiFiSSID() const
	{
		return _wifiSSID;
	}

	const std::string &DeviceSettings::getWiFiPassword() const
	{
		return _wifiPassword;
	}

	const std::string& DeviceSettings::getServerURL() const
	{
		return _serverURL;
	}

	void DeviceSettings::setWiFiSSID(const std::string &ssid)
	{
		_wifiSSID = ssid;
	}

	void DeviceSettings::setWiFiPassword(const std::string &password)
	{
		_wifiPassword = password;
	}

	void DeviceSettings::setServerURL(const std::string &url)
	{
		_serverURL = url;
	}

	bool DeviceSettings::getWiFiMACAddress(char *wifiMACString)
	{
		uint8_t		wifiMACAddress[MAC_ADDR_LEN] = {};

		if ( esp_read_mac(wifiMACAddress, ESP_MAC_WIFI_STA) != ESP_OK )
		{
			ESP_LOGE(LOG_TAG, "esp_read_mac() error");
			return false;
		}

		snprintf(wifiMACString, MAC_STR_LEN + 1, "%02X:%02X:%02X:%02X:%02X:%02X", wifiMACAddress[0], wifiMACAddress[1], wifiMACAddress[2], wifiMACAddress[3], wifiMACAddress[4], wifiMACAddress[5]);
		return true;
	}

	void DeviceSettings::loadDeviceShortID()
	{
		#if DEVICE_DEBUGGING == 1

			strncpy(_deviceShortID, DEBUGGING_DEVICE_ID, DEVICE_SHORT_ID_LENGTH + 1);

		#else

            const char *deviceID = DataStorage::getInstance().readTextFile(DEVICE_ID_FILE);

			if ( deviceID != nullptr && strlen(deviceID) == DEVICE_SHORT_ID_LENGTH )
			{
				strncpy(_deviceShortID, deviceID, DEVICE_SHORT_ID_LENGTH + 1);
			}
			else
			{
				generateDeviceShortID();
				storeDeviceShortID();
			}

			// deleting possible nullptr is safe
			delete deviceID;

		#endif

		ESP_LOGI(LOG_TAG, "Device-ID: %s", _deviceShortID);
	}

	void DeviceSettings::generateDeviceShortID()
	{
		uint8_t			wifiMACAddress[MAC_ADDR_LEN] = {};
		unsigned int	seed = 0;

		if ( esp_read_mac(wifiMACAddress, ESP_MAC_WIFI_STA) != ESP_OK )
		{
			ESP_LOGE(LOG_TAG, "esp_read_mac() error");
			wifiMACAddress[0] = 0x00;
			wifiMACAddress[1] = 0x11;
			wifiMACAddress[2] = 0x22;
			wifiMACAddress[3] = 0x33;
			wifiMACAddress[4] = 0x44;
			wifiMACAddress[5] = 0x55;
		}

		for(int i = 0; i < 6; i++)
		{
			unsigned long currByte = wifiMACAddress[i];
			currByte = currByte << (8 * i);
			seed |=  currByte;
		}

		srand(seed);

		for(int i = 0; i < DEVICE_SHORT_ID_LENGTH; i++)
		{
			uint8_t randomValue = static_cast<uint8_t>( random(0, 35) );
			char letter = static_cast<char>(randomValue) + 'A';
			if ( randomValue > 25 )
			{
				letter = static_cast<char>(randomValue - 26) + '0';
			}

			_deviceShortID[i] = letter;
		}
		_deviceShortID[DEVICE_SHORT_ID_LENGTH] = '\0';
	}

	void DeviceSettings::storeDeviceShortID()
	{
        DataStorage::getInstance().writeTextFile(DEVICE_ID_FILE, _deviceShortID);
	}

	void DeviceSettings::loadDeviceSettings()
	{
        unsigned char decryptionPass[ENCRYPTION_PASS_LEN + 1];
        getEncryptionPass(decryptionPass);

		cJSON *objectItem;

        const char *settingsJsonString = DataStorage::getInstance().readTextFile(DEVICE_SETTINGS_FILE);

        if ( settingsJsonString == nullptr )
        {
            return;
        }

        ESP_LOGW(LOG_TAG, "%s", settingsJsonString);

		cJSON *networkJson = cJSON_Parse(settingsJsonString);

		_wifiSSID       = "";
		_wifiPassword   = "";
		_serverURL      = "";

		if ( cJSON_IsObject(networkJson) )
		{
			objectItem = cJSON_GetObjectItem(networkJson, SSID_SETTINGS_NAME);
			if ( cJSON_IsString (objectItem) )
			{
                decryptionToStdString(objectItem->valuestring, decryptionPass, _wifiSSID);
				ESP_LOGI(LOG_TAG, "ssid loaded: %s", _wifiSSID.c_str() );
			}
			else
			{
				ESP_LOGE(LOG_TAG, "cJSON_IsString() for ssid failed");
			}

			objectItem = cJSON_GetObjectItem(networkJson, PASSWORD_SETTINGS_NAME);
			if ( cJSON_IsString (objectItem) )
			{
				decryptionToStdString(objectItem->valuestring, decryptionPass, _wifiPassword);
				ESP_LOGI(LOG_TAG, "password loaded: %s", _wifiPassword.c_str() );
			}
			else
			{
				ESP_LOGE(LOG_TAG, "cJSON_IsString() for password failed");
			}

			objectItem = cJSON_GetObjectItem(networkJson, "url");
			if ( cJSON_IsString (objectItem) )
			{
				_serverURL = objectItem->valuestring;
				ESP_LOGD(LOG_TAG, "url loaded: %s", _serverURL.c_str() );
			}
			else
			{
				ESP_LOGE(LOG_TAG, "cJSON_IsString() for url failed");
			}
		}
		else
		{
			ESP_LOGE(LOG_TAG, "cJSON_IsObject(networkJson) failed");
		}

		cJSON_free(networkJson);
		delete settingsJsonString;
	}

	void DeviceSettings::saveConfiguration()
	{
		unsigned char encryptionPass[ENCRYPTION_PASS_LEN + 1];
        getEncryptionPass(encryptionPass);

        unsigned char   ciphertext[MAX_ENCRYPTION_LEN];
        char            ciphertextString[MAX_ENCRYPTION_LEN * 2];
        uint8_t         ciphertextLen;

        ciphertextLen = encrypt(_wifiSSID.c_str(), _wifiSSID.length(), encryptionPass, ciphertext);

        for (int i = 0; i < ciphertextLen; i++)
        {
            sprintf(ciphertextString + (2*i), "%02X", *(ciphertext+i) );
        }

        cJSON *networkJson = cJSON_CreateObject();

		if ( cJSON_AddStringToObject(networkJson, SSID_SETTINGS_NAME, ciphertextString) == nullptr)
		{
			ESP_LOGE(LOG_TAG, "cJSON_AddStringToObject failed");
			cJSON_Delete(networkJson);
			return;
		}

		ciphertextLen = encrypt(_wifiPassword.c_str(), _wifiPassword.length(), encryptionPass, ciphertext);

        for (int i = 0; i < ciphertextLen; i++)
        {
            sprintf(ciphertextString + (2*i), "%02X", *(ciphertext+i) );
        }

		if ( cJSON_AddStringToObject(networkJson, PASSWORD_SETTINGS_NAME, ciphertextString ) == nullptr)
		{
			ESP_LOGE(LOG_TAG, "cJSON_AddStringToObject failed");
			cJSON_Delete(networkJson);
			return;
		}

		if ( cJSON_AddStringToObject(networkJson, "url", _serverURL.c_str() ) == nullptr)
		{
			ESP_LOGE(LOG_TAG, "cJSON_AddStringToObject failed - url");
			cJSON_Delete(networkJson);
			return;
		}

		char *jsonString = cJSON_Print(networkJson);
		ESP_LOGI(LOG_TAG, "%s", jsonString);

        ESP_LOGI(LOG_TAG, "Start saving config");
        int bytesWritten = DataStorage::getInstance().writeTextFile(DEVICE_SETTINGS_FILE, jsonString);

        if (bytesWritten > 0)
		{
			ESP_LOGD(LOG_TAG, "Network config saved...");
		}
        else
        {
            ESP_LOGE(LOG_TAG, "Saving config FAILED!");
        }

		// cJSON uses malloc(), don't mix malloc/new + free/delete
		free(jsonString);
		cJSON_Delete(networkJson);
	}

}
