# RevilMax
RevilMax is so far a RE Engine importer for 3ds Max. Buildable under VS2015.

Supported 3ds max versions: **2010 - 2019**

Tested on 3ds max versions: **2017**

## Editing .vcxproj
All essential configurations are within **PropertyGroup Label="MAXConfigurations"** field.
- **MaxSDK**: changes path where is your MAX SDK installation.
If your MAX SDK installation is somewhere else than default path stated in this field, you can edit it here.
- **MaxDebugConfiguration**: changes 3ds max version and platform, so all necessary files are copied into plugin directory, this will enable post-build event.

## Post-build event
Change 3ds max installation path (Default is C:\Program Files\Autodesk\). Do not change anything else unless you know what you're doing!

### [Latest Release](https://github.com/PredatorCZ/RevilMax/releases/)

## License
This plugin is available under GPL v3 license. (See LICENSE.md)

This plugin uses following libraries:

* RevilLib, Copyright (c) 2017-2019 Lukas Cone
