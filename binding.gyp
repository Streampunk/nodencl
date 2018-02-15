{
  "targets": [
    {
      "target_name": "nodencl",
      "sources": [ "nodencl.cc" ],
      "include_dirs": [ "include" ],
      "link_settings": {
        "libraries": [ "OpenCL.lib" ],
        "library_dirs": [ "lib/x64" ]
      }
    }
  ]
}
