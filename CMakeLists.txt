cmake_minimum_required(VERSION 3.12)

project(servant)
add_executable(servant main.cpp network.cpp os.cpp Resource.cpp Servant.cpp Session.cpp getopt.c)

if(WIN32)
	target_link_libraries(servant wsock32 ws2_32)
endif()
