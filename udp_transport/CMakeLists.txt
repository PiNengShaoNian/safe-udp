set(file
  data_segment.cpp
  )

add_library(udp_transport SHARED ${file})

install(TARGETS  udp_transport DESTINATION  ${PROJECT_BINARY_DIR}/lib)




