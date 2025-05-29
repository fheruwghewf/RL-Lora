

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
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK 
      ${liblorawan}
      ${Python3_LIBRARIES}
       # 根据需要添加其他依赖
      lorawan
  )
endforeach()
