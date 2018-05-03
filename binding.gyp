{
  "targets": [
    {
      "target_name": "nodencl",
      "sources": [ "src/nodencl.cc", "src/noden_util.cc", "src/noden_info.cc" ],
      "include_dirs": [ "include" ],
      "link_settings": {
        "libraries": [ "OpenCL.lib" ],
        "library_dirs": [ "lib/x64" ]
      }
    }
  ]
}
