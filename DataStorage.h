#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <string>


namespace _2log
{
    /**
     * @brief The DataStorage class provides a basic access to the SPIFFS device storage.
     */
	class DataStorage
	{
		public:

            /**
             * @brief Access to the single instance of the DataStore
             * @return the single instance of \c DataStorage
             */
            static DataStorage& getInstance();

            /**
             * @brief Mount the DataStorage on the specified path
             *
             * @param mountPoint    the mount point as path
             *
             * @return  true on success
             * @return  false on failure
             */
			bool			mount(const char* mountPoint);

            /**
             * @brief Unmount the DataStorage
             *
             * @return  true on success
             * @return  false on failure
             */
			bool			unmount(void);

            /**
             * @brief Read a text file from the DataStorage
             *
             * @param fileName  the file to read
             *
             * @return null-terminated string of the file contents
             */
			const char*		readTextFile(const char* fileName);

            /**
             * @brief Write a text file to the DataStorage
             *
             * @param fileName  the file to write
             * @param content   null-terminated string
             *
             * @return          >  \c 0 if write operation was successful, the value is the number of bytes actually written to the file
             * @return          <= \c 0 if the write operation failed
             */
			int				writeTextFile(const char* fileName, const char* content);

            /**
             * @brief Delete a file from the DataStorage
             *
             * @param fileName  the file to delete
             *
             * @return  true on success
             * @return  false on failure
             */
			bool			deleteFile(const char* fileName);

		private:

            DataStorage();
            DataStorage(DataStorage const&)     = delete;
            void operator=(DataStorage const&)  = delete;

            bool	format();
			bool	_isMounted;
	};
}

#endif
