cmake_minimum_required(VERSION 3.16)

project(fileopener VERSION 1.0.0 LANGUAGES CXX)

set(
	CMAKE_CXX_STANDARD 17
	CMAKE_CXX_STANDARD_REQUIRED ON
	CMAKE_EXPORT_COMPILE_COMMANDS ON
	CMAKE_BUILD_TYPE Debug
)

find_package(Qt6 6.5 COMPONENTS Quick REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(fileopener
	main.cpp
)

qt_add_qml_module(fileopener
	URI qrc
	QML_FILES
		main.qml
	RESOURCES
)

target_link_libraries(fileopener
	PRIVATE Qt6::Quick
)
