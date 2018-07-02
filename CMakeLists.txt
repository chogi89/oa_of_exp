cmake_minimum_required(VERSION 2.8.3)
project(oa_of_exp)

find_package(catkin REQUIRED COMPONENTS roscpp std_msgs message_generation geometry_msgs sensor_msgs)
find_package( OpenCV REQUIRED )

add_message_files(FILES MsgOAOF.msg)

generate_messages(DEPENDENCIES std_msgs geometry_msgs)

catkin_package(
  LIBRARIES oa_of_exp
  CATKIN_DEPENDS roscpp std_msgs cv_bridge image_transport geometry_msgs sensor_msgs
  DEPENDS system_lib
)

include_directories(include ${catkin_INCLUDE_DIRS})

add_executable(oa_of_exp_no_setpoint src/oa_of_exp_no_setpoint.cpp)
target_link_libraries(oa_of_exp_no_setpoint ${catkin_LIBRARIES} ${OpenCV_LIBRARIES})
add_dependencies(oa_of_exp_no_setpoint oa_of_exp_generate_messages_cpp)

add_executable(oa_of_exp_zero src/oa_of_exp_zero.cpp)
target_link_libraries(oa_of_exp_zero ${catkin_LIBRARIES} ${OpenCV_LIBRARIES})
add_dependencies(oa_of_exp_zero oa_of_exp_generate_messages_cpp)

add_executable(oa_of_exp_x src/oa_of_exp_x.cpp)
target_link_libraries(oa_of_exp_x ${catkin_LIBRARIES} ${OpenCV_LIBRARIES})
add_dependencies(oa_of_exp_x oa_of_exp_generate_messages_cpp)

add_executable(oa_of_exp_const_h src/oa_of_exp_const_h.cpp)
target_link_libraries(oa_of_exp_const_h ${catkin_LIBRARIES} ${OpenCV_LIBRARIES})
add_dependencies(oa_of_exp_const_h oa_of_exp_generate_messages_cpp)

add_executable(oa_of_exp_dc_const_h src/oa_of_exp_dc_const_h.cpp)
target_link_libraries(oa_of_exp_dc_const_h ${catkin_LIBRARIES} ${OpenCV_LIBRARIES})
add_dependencies(oa_of_exp_dc_const_h oa_of_exp_generate_messages_cpp)

add_executable(oa_of_exp_dc_no_setpoint src/oa_of_exp_dc_no_setpoint.cpp)
target_link_libraries(oa_of_exp_dc_no_setpoint ${catkin_LIBRARIES} ${OpenCV_LIBRARIES})
add_dependencies(oa_of_exp_dc_no_setpoint oa_of_exp_generate_messages_cpp)

add_executable(oa_of_exp_lt_const_h src/oa_of_exp_lt_const_h.cpp)
target_link_libraries(oa_of_exp_lt_const_h ${catkin_LIBRARIES} ${OpenCV_LIBRARIES})
add_dependencies(oa_of_exp_lt_const_h oa_of_exp_generate_messages_cpp)
