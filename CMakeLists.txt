# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES json main idfix-core idfix-protocols idfix-wifi idfix-crypto idfix-fota)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS	"BaseDevice.h" "BaseDevice.cpp"
			"IDeviceNode.h" "IDeviceNode.cpp"
			"DeviceNodeEventHandler.h" "DeviceNodeEventHandler.cpp"
			"DeviceNode.h" "DeviceNode.cpp"
			"StringComparison.h"
			"IConnection.h" "IConnection.cpp"
			"Connection.h" "Connection.cpp"
			"ConnectionEventHandler.h" "ConnectionEventHandler.cpp"
			"WMath.h" "WMath.cpp"
			"DataStorage.h" "DataStorage.cpp"
			"DeviceSettings.h" "DeviceSettings.cpp"
			"DeviceProperties.h" "DeviceProperties.cpp" )

set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_options(-std=gnu++17)
