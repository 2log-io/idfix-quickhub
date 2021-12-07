
#include "DeviceProperties.h"

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_system.h"



namespace
{
        const char* LOG_TAG = "_2log::DeviceProperties";
        const char* PREFIX  = "/2log/prop/"; // 11+1 chars
        const int MAX_PROPERTY_LENGTH = 45;
}

namespace _2log
{
        DeviceProperties::DeviceProperties()
        {
            init();
        }

        bool DeviceProperties::init()
        {
            _initialized = DataStorage::getInstance().mount("/2log");
            return _initialized;
        }

        bool DeviceProperties::saveProperty(const char *key, const DeviceProperties::PropertyValue& value)
        {
            if(!_initialized)
            {
                ESP_LOGD(LOG_TAG, "Not initialized.");
                return false;
            }

            if(strlen(PREFIX) + strlen(key) + 1 > MAX_PROPERTY_LENGTH)
            {
                  ESP_LOGD(LOG_TAG, "Property name has too much characters");
                  return false;
            }

            char filename[MAX_PROPERTY_LENGTH];
            strcpy(filename, PREFIX);
            strcat(filename, key);

            cJSON *object = cJSON_CreateObject();
            cJSON_AddNumberToObject(object, "type", (int) value.getType());

            switch(value.getType())
            {
                case PropertyValue::INT:
                case PropertyValue::FLOAT:
                    cJSON_AddNumberToObject(object, "val", value.asNumber(nullptr));
                    break;

                case PropertyValue::BOOL:
                    cJSON_AddBoolToObject(object, "val", value.asBool(nullptr));
                    break;

                case PropertyValue::CSTRING:
                    cJSON_AddStringToObject(object, "val", value.asCstring(nullptr));
                    break;

                default: return false;
            }

            char *jsonString = cJSON_Print(object);


            bool success = DataStorage::getInstance().writeTextFile(filename, jsonString) > 0;
            if(success)
            {
                ESP_LOGD(LOG_TAG, "property written: %s", filename);
                ESP_LOGD(LOG_TAG, "payload: %s", jsonString);
            }
            else
            {
                ESP_LOGD(LOG_TAG, "failed to write file: %s", filename);
            }

            // cJSON uses malloc(), don't mix malloc/new + free/delete
            free(jsonString);
            cJSON_Delete(object);
            return success;
        }
        
        bool DeviceProperties::deleteProperty(const char *key)
        {
            char filename[MAX_PROPERTY_LENGTH];

            if(strlen(PREFIX) + strlen(key) + 1 > MAX_PROPERTY_LENGTH)
            {
                  ESP_LOGD(LOG_TAG, "property name has too much characters");
                  return false;
            }
            strcpy(filename, PREFIX);
            strcat(filename, key);

            return DataStorage::getInstance().deleteFile(filename);
        }


        DeviceProperties::PropertyValue DeviceProperties::getProperty(const char *key, const PropertyValue &defaultValue)
        {
            if(!_initialized)
                return defaultValue;

            if(strlen(PREFIX) + strlen(key) + 1 > MAX_PROPERTY_LENGTH)
            {
                  ESP_LOGD(LOG_TAG, "property name has too much characters");
                  return {};
            }

            const char* propertyFile;

            {
                char filename[MAX_PROPERTY_LENGTH];
                strcpy(filename, PREFIX);
                strcat(filename, key);
                propertyFile = DataStorage::getInstance().readTextFile(filename);
            }

            cJSON* propertyJson = cJSON_Parse(propertyFile);

            PropertyValue property;
            if (propertyFile != nullptr)
            {

                cJSON* typeJSON = cJSON_GetObjectItem(propertyJson, "type");
                if(!(typeJSON && cJSON_IsNumber(typeJSON)))
                {
                    delete [] propertyFile;
                    cJSON_Delete(propertyJson);

                    return defaultValue;
                }

                PropertyValue::PropertyDataType type = static_cast<PropertyValue::PropertyDataType> (typeJSON->valueint);
                cJSON* valueJSON = cJSON_GetObjectItem(propertyJson, "val");

                switch(type)
                {
                    case PropertyValue::INT:
                        property.setInt(valueJSON->valueint);
                    break;

                    case PropertyValue::FLOAT:
                        property.setFloat(valueJSON->valuedouble);
                        break;

                    case PropertyValue::BOOL:
                        property.setBool(cJSON_IsTrue(valueJSON));
                        break;

                    case PropertyValue::CSTRING:
                        property.setString(valueJSON->string);
                        break;

                    default:;
                }
            }
            else
            {
                ESP_LOGD(LOG_TAG, "Property not available -> return default value");
                cJSON_Delete(propertyJson);
                return defaultValue;
            }

            cJSON_Delete(propertyJson);
            delete [] propertyFile;
            return property;
        }

        DeviceProperties& DeviceProperties::instance()
        {
            static DeviceProperties instance;
            return instance;
        }
}
