

set(base_examples
    simple-network-example
    network-server-example
    complete-network-example
    my-complete-network-example
    adr-example
    lorawan-energy-model-example
    aloha-throughput
    parallel-reception-example
    frame-counter-update
)



foreach(
  example
  ${base_examples}
  #my-complete-network-example
)
  find_package(Python3 REQUIRED COMPONENTS Development)
  include_directories(${Python3_INCLUDE_DIRS})
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK 
      ${liblorawan}
      ${Python3_LIBRARIES}
       # 根据需要添加其他依赖
      # lorawan
  )
  # target_link_libraries(${example} PRIVATE ${Python3_LIBRARIES})
endforeach()
