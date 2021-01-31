# Advanced Locomotion System V4 Refactored
![Image](https://github.com/Sixze/ALSRefactored/raw/main/Resources/Readme_Content_2.gif)

Completely refactored and bug fixed version of the C++ port of [Advanced Locomotion System V4](https://www.unrealengine.com/marketplace/en-US/product/advanced-locomotion-system-v1) for **Unreal Engine 4.26**.

## Supported Platforms
- Windows
- Linux

*Mac, Android, IOS, and console builds are not tested and supported at the moment. Use the plugin on those platforms with your own risk.*

## Features
- Full replication support with low bandwidth usage
- Plugin structure
- Highly optimized for production
- Lots of bug fixes additional to marketplace version

## Known Issues
- See [Issues](https://github.com/Sixze/ALSRefactored/issues) section

## Setting Up The Plugin
- Clone the repository inside your project's `Plugins` folder, or download the latest release and extract it into your project's `Plugins` folder.
- Put `Config/DefaultInput.ini` from the plugin folder inside your project's config folder. If your project already have this .ini file, merge it into yours.
- Add the lines below into your `DefaultEngine.ini`, below `[/Script/Engine.CollisionProfile]` tag (create the tag if it doesn't exist):
```
+DefaultChannelResponses=(Channel=ECC_GameTraceChannel2,DefaultResponse=ECR_Block,bTraceType=True,bStaticObject=False,Name="AlsClimbable")
+Profiles=(Name="AlsCharacter",CollisionEnabled=QueryAndPhysics,bCanModify=True,ObjectTypeName="Pawn",CustomResponses=((Channel="Visibility",Response=ECR_Ignore),(Channel="Camera",Response=ECR_Ignore)),HelpMessage="")
```
- Enable plugin in your project, generate visual studio project files and build. Your project needs to be a C++ project to build the plugin. Unfortunately, BP projects are not supported at the moment.
- Launch the project, and enable plugin content viewer as seen below. This will show contents of the plugin in your content browser:
![image](https://github.com/Sixze/ALSRefactored/raw/main/Resources/Readme_Content_1.png)

## License & Contribution
**Source code** of the plugin is licensed under MIT license, and other developers are encouraged to fork the repository, open issues & pull requests to help the development.
