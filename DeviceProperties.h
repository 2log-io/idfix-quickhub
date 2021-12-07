#ifndef DEVICEPROPERTIES_H
#define DEVICEPROPERTIES_H

#include <string>
#include "DataStorage.h"
#include <cJSON.h>


namespace _2log
{
    /**
     * @brief The DeviceProperties class provides a way to store properties as key/value pairs.
     */
    class DeviceProperties
    {
        public:

            /**
             * @brief The PropertyValue struct represents a variant property value
             */
            struct PropertyValue
            {
                /**
                 * @brief The PropertyDataType enum enumerates the possible data types
                 */
                enum PropertyDataType
                {
                    INVALID = -1,
                    INT = 0,
                    CSTRING,
                    FLOAT,
                    BOOL
                };

                /**
                 * @brief Initialize an invalid PropertyValue
                 */
                PropertyValue(void){}

                /**
                 * @brief Initialize an INT PropertyValue
                 * @param val   the int value
                 */
                PropertyValue(int val)
                {
                    data.val_int = val;
                    type = INT;
                }

                /**
                 * @brief Initialize a FLOAT PropertyValue
                 * @param val   the float value
                 */
                PropertyValue(float val)
                {
                    data.val_float = val;
                    type = FLOAT;
                }

                /**
                 * @brief Initialize a CSTRING PropertyValue
                 * @param val   the NULL-terminated c-string value
                 */
                PropertyValue(char* val)
                {
                    data.val_cstring = val;
                    type = CSTRING;
                }

                /**
                 * @brief Initialize a BOOL PropertyValue
                 * @param val   the bool value
                 */
                PropertyValue(bool val)
                {
                    data.val_bool = val;
                    type = BOOL;
                }

                /**
                 * @brief Returns the value as int
                 * @param success   is set to true if value could be converted to int
                 * @return  the value as int
                 */
                int asInt(bool* success) const
                {
                    if(success)
                        *success = (type == INT);

                    return data.val_int;
                }

                /**
                 * @brief Returns the value as float
                 * @param success   is set to true if value could be converted to float
                 * @return  the value as float
                 */
                float asFloat(bool* success) const
                {
                    if(success)
                        *success = (type == FLOAT);

                    return data.val_float;
                }

                /**
                 * @brief Returns the value as number
                 * @param success   is set to true if value could be converted to number
                 * @return  the value as number
                 */
                float asNumber(bool* success) const
                {
                    if(success)
                        *success = (type == FLOAT || type == INT);

                    if (type == FLOAT)
                        return data.val_float;
                    else
                        return data.val_int;
                }

                /**
                 * @brief Returns the value as c-string
                 * @param success   is set to true if value could be converted to c-string
                 * @return  the value as c-string
                 */
                char* asCstring(bool* success) const
                {
                    if(success)
                        *success = (type == CSTRING);

                    return data.val_cstring;
                }

                /**
                 * @brief Returns the value as bool
                 * @param success   is set to true if value could be converted to bool
                 * @return  the value as bool
                 */
                bool asBool(bool* success) const
                {
                    if(success)
                        *success = (type == BOOL);

                    return data.val_bool;
                }

                /**
                 * @brief Set the value to a c-string
                 * @param val   the c-string value
                 */
                void setString(char* val)
                {
                    type = CSTRING;
                    data.val_cstring = val;
                }

                /**
                 * @brief Set the value to a bool
                 * @param val   the bool value
                 */
                void setBool(bool val)
                {
                    type = BOOL;
                    data.val_bool = val;
                }

                /**
                 * @brief Set the value to a float
                 * @param val   the float value
                 */
                void setFloat(float val)
                {
                    type = FLOAT;
                    data.val_float = val;
                }

                /**
                 * @brief Set the value to an int
                 * @param val   the int value
                 */
                void setInt(int val)
                {
                    type = INT;
                    data.val_int = val;
                }

                /**
                 * @brief Get the value data type
                 * @return
                 */
                PropertyDataType getType() const
                {
                    return type;
                }

            private:
                union PropertyData
                {
                    int val_int;
                    float val_float;
                    char* val_cstring;
                    bool  val_bool;
                } data;

                PropertyDataType type = INVALID;
            };

            /**
             * @brief Saves a property to the storage
             * @param key   the property key as NULL-terminated c-string
             * @param value the value for this property
             *
             * @return  \c true if property was successfully stored, \c false otherwise
             */
            bool saveProperty(const char* key, const PropertyValue &value);

            /**
             * @brief Delete a property from the storage
             * @param key   the property key as NULL-terminated c-string
             * @return  \c true if property was successfully deleted, \c false otherwise
             */
            bool deleteProperty(const char* key);

            /**
             * @brief Loads a property from the storage
             * @param key           the property key as NULL-terminated c-string
             * @param defaultValue  a default value that is used, if the property did not yet exist
             * @return  the loaded property
             */
            PropertyValue getProperty(const char* key, const DeviceProperties::PropertyValue& defaultValue = {});

            /**
             * @brief Access to the single instance of the DeviceProperties
             * @return the single instance of \c DeviceProperties
             */
            static DeviceProperties& instance();

        private:
            bool init();
            bool _initialized = false;
            DeviceProperties();
    };
}

#endif
