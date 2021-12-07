#ifndef DEVICESETTINGS_H
#define DEVICESETTINGS_H

#include "DataStorage.h"

#define DEVICE_SHORT_ID_LENGTH		4
#define MAC_ADDR_LEN                6
#define MAC_STR_LEN                 17
#define ENCRYPTION_PASS_LEN         21
#define MAX_ENCRYPTION_LEN          64

namespace _2log
{
    /**
     * @brief The DeviceSettings class manages the settings of the device.
     */
	class DeviceSettings
	{
		public:

            /**
             * @brief Constructs a new DeviceSettings object
             */
									DeviceSettings();

            /**
             * @brief Initialize and load the settings
             * @return  \c true if setting were successfully initialized
             */
			bool					init(void);

            /**
             * @brief Checks if the device has already stored settings
             *
             * @return  \c true if device has already stored settings
             * @return  \c false if device is not yet configured
             */
			bool					isConfigured(void) const;

            /**
             * @brief Delete the stored WiFi and URL configuration
             * @return  \c true if configuration was successfully deleted
             * @return  \c false if configuration could not be deleted
             */
			bool					clearConfig(void);

            /**
             * @brief Get the short ID of this device
             * @return  the short ID
             */
			const char*				getShortID(void) const;

            /**
             * @brief Get the unique ID of this device
             * @return the unique device ID
             */
			std::string				getDeviceID() const;

            /**
             * @brief Get the authentication key of this device
             * @return  the authentication key
             */
			uint32_t				getAuthKey(void) const;

            /**
             * @brief Write new authentication key to storage
             * @param authKey   the new authentication key
             */
			void					writeAuthKey(uint32_t authKey);

            /**
             * @brief Get the cunfigured WiFi SSID
             * @return  the WiFi SSID
             */
			const std::string&		getWiFiSSID(void)		const;

            /**
             * @brief Get the cunfigured WiFi password
             * @return  the WiFi password
             */
			const std::string&		getWiFiPassword(void)	const;

            /**
             * @brief Get the cunfigured QuickHub server URL
             * @return  the QuickHub server URL
             */
			const std::string&		getServerURL(void)		const;

            /**
             * @brief Set a new WiFi SSID
             * @param ssid  the new WiFi SSID
             */
			void					setWiFiSSID(const std::string&		ssid);

            /**
             * @brief Set a new WiFi password
             * @param password  the new WiFi password
             */
			void					setWiFiPassword(const std::string&	password);

            /**
             * @brief Set a new QuickHub server URL
             * @param url   the new QuickHub server URL
             */
			void					setServerURL(const std::string&		url);

            /**
             * @brief Save the WiFi and server URL configuration to the storage.
             */
			void					saveConfiguration(void);

            /**
             * @brief Get the WiFi station MAC address
             * @param wifiMACString buffer to store the MAC address as string
             *
             * @return  \c true if MAC address was successfully copied to buffer
             * @return  \c false if MAC address could not be obtained
             */
			bool					getWiFiMACAddress(char *wifiMACString);

		private:

            /**
             * @brief Load the short ID from the storage
             */
			void			loadDeviceShortID(void);

            /**
             * @brief Generate a new short ID for this device
             */
			void			generateDeviceShortID(void);

            /**
             * @brief Persist the short ID on the storage.
             */
			void			storeDeviceShortID(void);

            /**
             * @brief Load the wifi and server URL from the storage
             */
			void			loadDeviceSettings(void);

            /**
             * @brief Load the auth key from the storage.
             */
			void			loadAuthKey(void);

            /**
             * @brief Get the encryption password for this device.
             * @param encryptionPass    the buffer to store the password
             */
			void            getEncryptionPass(unsigned char encryptionPass[ENCRYPTION_PASS_LEN + 1]);

            /**
             * @brief Encrypt the input with AES-256
             * @param input             the input clear text
             * @param inputLength       the length of the input in bytes
             * @param encryptionPass    the encryption password
             * @param ciphertext        buffer to store the ciphertext
             *
             * @return  the length of the (possible padded) ciphertext
             */
			uint8_t         encrypt(const char* input, size_t inputLength, const unsigned char* encryptionPass, unsigned char ciphertext[MAX_ENCRYPTION_LEN]);

            /**
             * @brief Decrypt the input with AES-256
             * @param ciphertext        the input ciphertext
             * @param ciphertextLen     the length of the input in bytes
             * @param decryptionPass    the decryption password
             * @param cleartext         buffer to store the decrypted cleartext
             */
			void            decrypt(const unsigned char* ciphertext, uint8_t ciphertextLen, const unsigned char* decryptionPass, unsigned char cleartext[MAX_ENCRYPTION_LEN]);

            /**
             * @brief Decrypt the HEX string encoded ciphertext
             * @param ciphertextHexString   the ciphertext as HEX decoded string
             * @param decryptionPass        the decryption password
             * @param output                buffer to store the decrypted cleartext
             */
			void            decryptionToStdString(const char* ciphertextHexString, const unsigned char* decryptionPass, std::string &output);

		private:

            char            _deviceShortID[DEVICE_SHORT_ID_LENGTH+1];
			uint32_t		_authKey = {0};
			std::string		_wifiSSID;
			std::string		_wifiPassword;
			std::string		_serverURL;
	};
}

#endif
