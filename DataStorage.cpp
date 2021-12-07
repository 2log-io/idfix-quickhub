#include "DataStorage.h"

extern "C"
{
	#include "esp_spiffs.h"
	#include "esp_log.h"
	#include "esp_task_wdt.h"
	#include "string.h"
	#include <sys/stat.h>
	#include <stdio.h>
	#include <sys/unistd.h>

    #ifndef CONFIG_IDF_TARGET_ESP32
    #include "FreeRTOS.h"
    #include "freertos/task.h"
    #endif
}

namespace
{
	const char* LOG_TAG = "_2log::DataStorage";
}

namespace _2log
{
	DataStorage::DataStorage() : _isMounted(false)
	{

	}

    DataStorage &DataStorage::getInstance()
    {
        static DataStorage instance;
        return instance;
    }

    bool DataStorage::mount(const char* mountPoint)
	{
		if ( _isMounted || esp_spiffs_mounted(nullptr) )
		{
			ESP_LOGW(LOG_TAG, "SPIFFS Already Mounted!");
			return true;
		}

		esp_vfs_spiffs_conf_t conf = {};

		conf.base_path				= mountPoint;
		conf.partition_label		= nullptr;
		conf.max_files				= 10;
		conf.format_if_mount_failed	= false;

		esp_err_t err = esp_vfs_spiffs_register(&conf);
        if(err == ESP_FAIL)
		{
			// automatically format on failure (first use)
			if( format() )
			{
				err = esp_vfs_spiffs_register(&conf);
			}
		}
		if(err != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "Mounting SPIFFS failed! Error: %d", err);
			return false;
		}

		_isMounted = true;

		size_t total = 0, used = 0;

		err = esp_spiffs_info(nullptr, &total, &used);
		if (err != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(err) );
		}
		else
		{
			ESP_LOGI(LOG_TAG, "Partition size: total: %zu, used: %zu", total, used);
		}

		return true;
	}

	bool DataStorage::unmount()
	{
		if( esp_spiffs_mounted(nullptr) )
		{
			esp_err_t err = esp_vfs_spiffs_unregister(nullptr);
			if(err)
			{
				ESP_LOGE(LOG_TAG, "Unmounting SPIFFS failed! Error: %d", err);
				return false;
			}
		}
		_isMounted = false;
		return true;
	}

	const char *DataStorage::readTextFile(const char *fileName)
	{
		ESP_LOGD(LOG_TAG, "Reading file %s", fileName);
		struct stat fileStat;
		long		fileSize;
		char*		fileBuffer = nullptr;

		if ( ! _isMounted )
		{
			ESP_LOGE(LOG_TAG, "Failed to read file: SPIFFS not mounted");
			return nullptr;
		}

		if ( stat(fileName, &fileStat) != 0)
		{
			// file does not exists
			ESP_LOGE(LOG_TAG, "File %s does not exist", fileName);
			return nullptr;
		}

		FILE* file = fopen(fileName, "r");
		if (file == nullptr)
		{
			ESP_LOGE(LOG_TAG, "Failed to open file for reading");
			return nullptr;
		}

		fseek (file , 0 , SEEK_END);
		fileSize = ftell(file);
		rewind(file);

		if ( fileSize == 0 )
		{
			ESP_LOGE(LOG_TAG, "File %s is empty", fileName);
			fclose(file);
			return nullptr;
		}

		fileBuffer = new char [fileSize + 1];

		if ( fileBuffer == nullptr )
		{
			ESP_LOGE(LOG_TAG, "Memory error");
			fclose(file);
			return nullptr;
		}

		size_t result = fread (fileBuffer, 1, static_cast<size_t>(fileSize), file);

		if ( result != static_cast<size_t>(fileSize) )
		{
			ESP_LOGE(LOG_TAG, "reading error");
			delete fileBuffer;
			fileBuffer = nullptr;
		}
		else
		{
			fileBuffer[fileSize] = '\0';
		}

		fclose(file);

		return fileBuffer;
	}

	int DataStorage::writeTextFile(const char *fileName, const char *content)
	{
		ESP_LOGD(LOG_TAG, "Writing file %s", fileName);

		if ( ! _isMounted )
		{
			ESP_LOGE(LOG_TAG, "Failed to write file: SPIFFS not mounted");
			return -1;
		}

		FILE* file = fopen(fileName, "w");

		if (file == nullptr)
		{
			ESP_LOGE(LOG_TAG, "Failed to open file for writing");
			return -1;
		}

		int charsWritten = fprintf(file, "%s", content);
		fclose(file);

		ESP_LOGD(LOG_TAG, "File %s written", fileName);
		return charsWritten;
	}

	bool DataStorage::deleteFile(const char *fileName)
	{
		struct stat fileStats;
		bool fileWasOverwritten = false;
		if ( stat(fileName, &fileStats) == 0 )
		{
			// complete erase file contents
			FILE* file = fopen(fileName, "w");

			for (int i = 0; i < fileStats.st_size; i++)
			{
				fwrite(" ", 1, 1, file);
			}

			fclose(file);
			fileWasOverwritten = true;
		}

		// finally delete file
		return unlink(fileName) == 0 || fileWasOverwritten;
	}

	bool DataStorage::format()
	{

        ESP_LOGI(LOG_TAG, "Format file system start..");

        #ifdef CONFIG_IDF_TARGET_ESP32

            UBaseType_t coreID = xPortGetCoreID();
            TaskHandle_t idleTask = xTaskGetIdleTaskHandleForCPU( coreID );

            // disable watchdog for current core, as esp_spiffs_format can take some time
            if( idleTask == nullptr || esp_task_wdt_delete(idleTask) != ESP_OK )
            {
                ESP_LOGE(LOG_TAG, "Failed to remove Core %d IDLE task from WDT", coreID);
            }

        #endif

            esp_err_t err = esp_spiffs_format(nullptr);

		// re-enable watchdog
        #ifdef CONFIG_IDF_TARGET_ESP32

            if( idleTask == nullptr || esp_task_wdt_add(idleTask) != ESP_OK )
            {
                ESP_LOGE(LOG_TAG, "Failed to add Core %d IDLE task to WDT", coreID);
            }

        #endif

		if(err)
		{
			ESP_LOGE(LOG_TAG, "Formatting SPIFFS failed! Error: %d", err);
			return false;
		}

        ESP_LOGI(LOG_TAG, "Format file system finished..");

		return true;
	}
}
