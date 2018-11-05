{
  "targets": [
    {
      "target_name": "nodencl",
      "sources": [
        "src/nodencl.cc",
        "src/noden_util.cc",
        "src/noden_info.cc",
        "src/noden_context.cc",
        "src/noden_program.cc",
        "src/noden_buffer.cc",
        "src/noden_run.cc",
        "src/cl_memory.cc"
      ],
      "include_dirs": [ "include" ],
      "link_settings": {
        "libraries": [ "OpenCL.lib" ],
        "library_dirs": [ "lib/x64" ]
      }
    }
  ]
}
