project(fbcp)
cmake_minimum_required(VERSION 2.8)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION "1.0.0")

set(COMPILE_DEFINITIONS -Werror)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

link_directories(/opt/vc/lib)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} bcm_host)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES README.md DESTINATION share/${PROJECT_NAME})
