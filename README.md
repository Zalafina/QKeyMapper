# QKeyMapper
---------------
## 在Win10和Win11下可以正常使用的按键映射工具，使用Qt Widget + WinAPI开发，v1.2.8(2022-12-24)开始更新了对Qt6的支持，v1.2.8及之后的Release中使用Qt6编译的版本，v1.3.6(Build 20231220)新增游戏手柄按键和虚拟游戏手柄支持。
---------------
### 最新Release版本压缩包下载:
### [![Github Latest Release](https://img.shields.io/github/downloads/Zalafina/QKeyMapper/total.svg)](https://github.com/Zalafina/QKeyMapper/releases/latest)
### https://github.com/Zalafina/QKeyMapper/releases/latest
### https://gitee.com/asukavov/QKeyMapper/releases/latest
### ※ QKeyMapper_vX.Y.Z_x64开头的ZIP包是编译好的可执行文件压缩包
---------------
### 许可证更改 ：请注意，软件许可证已从 MIT 许可证更改为 GNU 通用公共许可证 v3.0 (GPL v3)。在使用、修改或分发软件之前，请确保你已经查看并理解了新许可证的条款。
### Win10及Win11系统推荐使用Qt6版本，提供Qt5版本只是为了能兼容Win7系统。Win7下如果无法使用Qt6版本的话请下载Qt5版本使用。
### 注意: 使用时需要 Visual C++ Redistributable 64位运行库。<br>VC++ 2015-2022 64位运行库，微软下载网址：<br>https://aka.ms/vs/17/release/vc_redist.x64.exe
---------------
## 使用教学视频请点击下方图片
[![](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/show_video.png)](https://www.bilibili.com/video/BV1Re411f7YS/?share_source=copy_web&vd_source=9602438c12e2a27bbe08c10cd8723515)
---------------
## 使用中有疑问也可以加Q群咨询 (群号: 906963961)
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_QGroup_Number.png)
---------------
### 新添加功能列表(根据更新时间降序排列)
* v1.3.6(Build 20240223)
    * 【极限竞速: 地平线】键盘可使用虚拟手柄 "vJoy-Key11(LT)_BRAKE" 映射来根据 "刹车阈值" 自动控制刹车力度。
    * 更改组合键映射策略，"原始组合键"改为文本框来手动输入，现在可以支持 [CombinationSupportKeys.txt 文件的按键列表](https://github.com/Zalafina/QKeyMapper/wiki/CombinationSupportKeys)中列出的所有按键进行组合，支持键盘按键与鼠标按键组合使用。支持的按键列表参考版本压缩包中的 CombinationSupportKeys.txt 文件，编写时候按键之间用"+"连接，例如："L-Ctrl+Home"、"A+B+C"。
    * "显示切换键"和"映射开关键"热键也改为文本框来手动输入。
    * 删除"禁用Win键"勾选框，现在可以通过映射按键中选择"BLOCKED"来禁用任意按键。

#### ※ 注意: v1.3.6(Build 20240221)对部分按键名称进行了更改，可能出现无法加载旧版本配置文件的情况，建议更新此版本前备份保存可执行文件路径下的 keymapdata.ini 配置文件。可以根据新Release版本ZIP压缩包中的 CombinationSupportKeys.txt 文件中的按键名称编辑修改 keymapdata.ini 配置文件中原来的按键名称来继续使用旧配置文件。
* v1.3.6(Build 20240131)
    * UI控件整体布局变化。
    * 增加虚拟手柄类型选择(X360/DS4), 如果使用以前版本的设定文件时提示"从INI文件加载了无效的设定数据"，在程序路径下备份keymapdata.ini后，使用文本编辑工具对keymapdata.ini进行文本替换后可继续使用。替换内容：(A) -> (A/×)、(B) -> (B/○)、(X) -> (X/□)、(Y) -> (Y/△)
* v1.3.6(Build 20240125)
    * 增加了窗口显示切换键编辑框，可以用于更改显示和隐藏窗口到系统托盘的快捷键。
    * 用于匹配窗口标题的单行文本编辑框可以手动进行内容更改并保存到设定了，同一个进程可以保存多个不同的窗口标题设定了(同一进程最多保存9个不同标题的设定)，不同的标题会保存到<process.exe|TitleX>形式的设定中。再次保存同一进程名相同标题的设定会覆盖之前已经存在的设定。
    * 双击左侧进程列表后，如果进程名和标题与已经保存的设定完全匹配，会自动加载显示之前已经保存过的设定。
    * 界面上控件位置微调，设定选择列表放在了进程名和标题名文本框的下面。给虚拟手柄设定添加了GroupBox框。
    * 窗口可见状态下，每隔3秒自动刷新左侧显示的进程列表。
    * ADD按键右侧"»"勾选框添加按键序列可以支持"vJoy"开头的虚拟手柄按键。
    * "保存设定"按钮可以保存当前窗口位置，隐藏后重新显示窗口会显示在隐藏前的窗口位置。程序启动时按照上次保存设定时的窗口位置进行显示。
    * 使用"»"勾选框添加按键序列可以使用延时功能了。
    * 窗口标题加入Build Number显示。
    * 增加"音效"复选框，勾选后开始映射和停止映射时会播放音效。
    * 增加游戏手柄左/右摇杆控制鼠标指针的功能(Joy-LS2Mouse & Joy-RS2Mouse)。
* v1.3.6(Build 20240112)
    * 增加了固定的全局映射设定项(QKeyMapperGlobalSetting)，如果全局映射勾选了自动开始映射，开始映射状态下从匹配的窗口退出后过几秒时间会自动切换到全局映射设定并启用映射。为全局映射状态加了一个专门的托盘图标。
* v1.3.6(Build 20240106)
    * 增加了原始快捷键编辑框，可以将包含Ctrl、Shift、Alt按键的组合键设定为原始输入(原始按键下拉框为空时，才会添加原始快捷键编辑框中的设定按键)。
    * 增加了按键映射停止时的音效。
* v1.3.6(Build 20231230)
    * 为映射按键增加"延时"数值调节框，可以适当增加组合按键之间的按下和抬起时的等待时间。
* v1.3.6(Build 20231225)
    * 添加"锁定光标"复选框，勾选后鼠标控制虚拟游戏手柄摇杆时鼠标光标被锁定在屏幕右下角位置(使用此功能前请提前确认映射开关组合键可用，避免鼠标无法移动也无法关闭按键映射恢复鼠标移动)。
    * 添加鼠标滚轮上滚和下滚映射功能。
* v1.3.6(Build 20231223)
    * 添加了通过鼠标控制虚拟游戏手柄左/右摇杆的功能(Mouse2Joystick)，通过在映射表中添加"vJoy-Mouse2LS"或"vJoy-Mouse2RS"来使用。鼠标控制摇杆的X轴和Y轴灵敏度范围"1~1000"，数值越小越灵敏。
    * "自动映射并最小化" 按钮变为三态复选框，设置为中间状态时软件启动仅最小化到托盘，不自动开始按键映射。
* v1.3.6(Build 20231220)
    * 添加了虚拟游戏手柄功能(通过ViGEmBus实现)，点击"安装ViGEmBus"，显示绿色字"ViGEmBus可用"之后，勾选"启用虚拟手柄"，之后可以将键盘按键映射到"vJoy"开头的虚拟手柄按键。
* v1.3.6(Build 20231125)
    * 添加了将游戏手柄按键作为原始按键映射到键盘按键的功能，原始按键列表中选择"Joy"开头的按键，映射按键选择想触发的键盘按键即可。
* v1.3.5(Build 20230806)
    * 切换映射开始和停止的快捷键，可以通过KeySequenceEdit控件进行自定义设置，鼠标点击KeySequenceEdit控件后按下想要设置的快捷键，此自定义快捷键设置每个配置可以保存设置不同值。
* v1.3.5(Build 20230805)
    * 添加可以直接在任意状态下切换映射开始和停止的快捷键，按下"Ctrl + F6"快捷键，无论在前台或者托盘显示状态都可以在开始映射和停止映射状态之间立即切换。
* v1.3.5
    * 添加中文界面，可以使用语言切换下拉列表进行中英文界面切换，适配4K/2K/1K分辨率。
* v1.3.3
    * 按键列表中添加了对鼠标侧键XButton1和XButton2的支持。
* v1.3.3
    * 在ADD按键右侧增加了"»"勾选框，用于添加按键序列，例如: Ctrl + X 之后 Ctrl + S，设置后效果参考README中显示的截图。
* v1.3.2
    * 在"KeyMappingStart"(循环检测)状态下，如果检测到到前台窗口与当前SelectSetting的按键映射配置设定一致时候会播放一个音效提示进入到按键映射生效状态。
* v1.3.1
    * 加入单例进程支持，只能同时运行一个QKeyMapper程序, 运行第二个QKeyMapper程序时会将第一个运行的实例程序窗口显示到前台。
* v1.3.1
    * "Auto Startup"勾选框可以让QKeyMapper按键映射程序在Windows用户登录的时候自动启动，配合"Auto Start Mapping"勾选框开机启动后可以自动最小化到系统托盘图标，取消勾选框则取消开机登录自动启动。
* v1.3.0
    * 开始支持"SaveMapData"可以将多个程序的不同按键映射配置都保存到"keymapdata.ini"文件中。如果配置了"Auto Start Mapping"功能，那么在"KeyMappingStart"状态下会根据当前前台窗口的进程可执行文件名进行自动匹配切换到对应的按键映射配置。
---------------
### 基本功能列表
1. 左侧显示当前正在运行的可见的窗口进程列表，进程可执行exe文件的文件名和对应的窗口标题名都会显示出来，信息会用于匹配当前处于前台的窗口。
2. 鼠标左键双击左侧窗口程序列表的某一行可以将选定的窗口程序的可自行文件名和窗口标题设定到图标/文件名/窗口标题区域。
3. 按下"KeyMappingStart"按钮可以开始执行按键映射处理流程，映射工具会定时循环检测当前处于前台的窗口与设定的可自行文件名和窗口标题是否匹配，按键映射功能可以只在前台窗口与当前进程可执行文件名和窗口标题都匹配的情况才生效，前台窗口不匹配的情况下按键映射自动失效。
4. 支持在右侧添加和删除按键映射表，下拉列表获取焦点时按下特定键盘按键可以自动将按键名设置到下拉列表上。
5. "SaveMapData"按钮会将按键映射表中的信息和进程及窗口标题信息保存到同一路径下的ini设定文件中，之后按键映射工具下次启动时可以自动加载同一路径下的"keymapdata.ini"文件读取已保存的设定值。
6. 按下"Ctrl + `"组合键会自动将QKeyMapper最小化显示到任务栏的系统程序托盘上，左键双击程序托盘图标会恢复显示程序窗口。
7. 将多个按键映射到同一个原始按键会按照"A + B"的形式来显示在按键映射表中，以此来支持用一个原始按键来同时映射多个按键。
8. 加入了"禁用WIN键"功能可以让键盘上的Windows按键在游戏中无效。
9. 右侧按键映射表中加入了连发(Burst)选择框，可以按照设定的连发按下时间(BurstPress)和连发抬起时间(BurstRelease)设定来反复发送映射表中的按键。
10. 右侧按键映射表中加入了锁定(Lock)选择框，选择后第一次按下对应的原始按键后会保持此按键的按下状态，再次按下此原始按键后接触锁定状态，支持与连发(Burst)选择框来组合使用，映射表中的Lock列的ON和OFF表示当前按键锁定状态。
11. 加入了"Auto Start Mapping"功能可以让程序启动后自动根据加载的配置文件开始按键映射，并且最小化到系统托盘, 。
12. 增加了SelectSetting下拉列表，可以保存最多10组不同的设定，并且通过下拉列表随时切换。
13. 增加了支持将鼠标左键&右键&中键映射到各种按键。
---------------
## Screenshot
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_screenshot_02.png)
---------------
## VirScan result
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_VirScan.png)
