{
  "targets": [
    {
      "target_name": "nodencl",
      "sources": [ "src/nodencl.cc", "src/noden_util.cc" ],
      "include_dirs": [ "include" ],
      "link_settings": {
        "libraries": [ "OpenCL.lib" ],
        "library_dirs": [ "lib/x64" ]
      }
    }
  ]
}
