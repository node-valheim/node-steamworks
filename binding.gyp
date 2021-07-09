{
    "targets": [{
        "target_name": "steamworks",
        "cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
        "sources": [
            "cppsrc/main.cpp",
            "cppsrc/GameNetworkingServer/GameNetworkingServerExport.cpp",
            "cppsrc/GameNetworkingServer/GameNetworkingServerWrapper.cpp",
            "cppsrc/GameNetworkingClient/GameNetworkingClientExport.cpp",
            "cppsrc/GameNetworkingClient/GameNetworkingClientWrapper.cpp",
        ],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")",
            '<(module_root_dir)/include'
        ],
        'dependencies': [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        'conditions': [
            ['OS=="mac"', {
                'xcode_settings': {
                    'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
                },
                'libraries': [
                    '<(module_root_dir)/lib/osx/libsteam_api.dylib', 
                ],
            }],
            ['OS=="linux"', {
                'libraries': [
                    '<(module_root_dir)/lib/linux64/libsteam_api.so', 
                ],
            }],
            ['OS=="windows"', {
                'libraries': [
                    '<(module_root_dir)/lib/win64/steam_api.lib', 
                ],
            }]
        ],
        'defines': [ 'NAPI_CPP_EXCEPTIONS' ]
    },{
         "target_name": "copy_binary",
         "type":"none",
         "dependencies" : [ "steamworks" ],
         'conditions': [
            ['OS=="mac"', {
                "copies":
                [
                    {
                    'destination': '<(module_root_dir)/build/Debug',
                    'files': [
                        '<(module_root_dir)/lib/osx/libsteam_api.dylib',
#                        '<(module_root_dir)/build/Debug/steamworks.node',
#                        '<(module_root_dir)/build/Release/steamworks.node',
                        ]
                    },
                    {
                        'destination': '<(module_root_dir)/build/Release',
                        'files': [
                            '<(module_root_dir)/lib/osx/libsteam_api.dylib',
    #                        '<(module_root_dir)/build/Debug/steamworks.node',
    #                        '<(module_root_dir)/build/Release/steamworks.node',
                            ]
                        },
                ]
            }],
            ['OS=="linux"', {
                "copies":
                [
                    {
                    'destination': '<(module_root_dir)/build/',
                    'files': [
                        '<(module_root_dir)/lib/linux64/libsteam_api.so',
#                        '<(module_root_dir)/build/Debug/steamworks.node',
                        '<(module_root_dir)/build/Release/steamworks.node',
                        ]
                    },
                ]
            }],
            ['OS=="windows"', {
                "copies":
                [
                    {
                    'destination': '<(module_root_dir)/build/',
                    'files': [
                        '<(module_root_dir)/lib/win64/steam_api64.dll',
#                        '<(module_root_dir)/build/Debug/steamworks.node',
                        '<(module_root_dir)/build/Release/steamworks.node',
                        ]
                    },
                ]
            }]
         ]
      }]
}
