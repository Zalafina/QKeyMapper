[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$uiPath = Join-Path $repoRoot "QKeyMapper\qkeymapper.ui"
$modernUiPath = Join-Path $repoRoot "QKeyMapper\qkeymapper_modern.ui"

# Load original XML
[xml]$doc = Get-Content $uiPath -Raw -Encoding UTF8

$cw = $doc.ui.widget.widget | Where-Object { $_.name -eq "centralwidget" }
$directWidgets = @($cw.widget)

# Index widgets by name
$widgetMap = @{}
foreach ($w in $directWidgets) {
    $widgetMap[$w.name] = $w
}

Write-Host "Found $($widgetMap.Count) direct widgets under centralwidget."

# Helper to remove geometry property from a widget node so QLayout manages it
function Clear-Geometry($w) {
    if (-not $w) { return }
    $geo = $w.property | Where-Object { $_.name -eq "geometry" }
    if ($geo) {
        [void]$w.RemoveChild($geo)
    }
}

function Set-FixedSize($w, [int]$width, [int]$height) {
    if (-not $w) { return }
    $min = $w.property | Where-Object { $_.name -eq "minimumSize" }
    if ($min) { [void]$w.RemoveChild($min) }
    $max = $w.property | Where-Object { $_.name -eq "maximumSize" }
    if ($max) { [void]$w.RemoveChild($max) }

    $minElem = $w.OwnerDocument.CreateElement("property")
    $minElem.SetAttribute("name", "minimumSize")
    $minElem.InnerXml = "<size><width>$width</width><height>$height</height></size>"
    [void]$w.AppendChild($minElem)

    $maxElem = $w.OwnerDocument.CreateElement("property")
    $maxElem.SetAttribute("name", "maximumSize")
    $maxElem.InnerXml = "<size><width>$width</width><height>$height</height></size>"
    [void]$w.AppendChild($maxElem)
}

function Set-FixedHeight($w, [int]$height) {
    if (-not $w) { return }
    $min = $w.property | Where-Object { $_.name -eq "minimumSize" }
    if ($min) { [void]$w.RemoveChild($min) }
    $max = $w.property | Where-Object { $_.name -eq "maximumSize" }
    if ($max) { [void]$w.RemoveChild($max) }

    $minElem = $w.OwnerDocument.CreateElement("property")
    $minElem.SetAttribute("name", "minimumSize")
    $minElem.InnerXml = "<size><width>0</width><height>$height</height></size>"
    [void]$w.AppendChild($minElem)

    $maxElem = $w.OwnerDocument.CreateElement("property")
    $maxElem.SetAttribute("name", "maximumSize")
    $maxElem.InnerXml = "<size><width>16777215</width><height>$height</height></size>"
    [void]$w.AppendChild($maxElem)
}

# Clear geometries of direct widgets
foreach ($w in $directWidgets) {
    Clear-Geometry $w
}

# Enforce fixed sizes on specific buttons and fixed controls
Set-FixedSize $widgetMap["keymapButton"] 171 51
Set-FixedSize $widgetMap["addmapdataButton"] 81 36
Set-FixedSize $widgetMap["backupSettingButton"] 71 22
Set-FixedSize $widgetMap["savemaplistButton"] 111 31
Set-FixedSize $widgetMap["originalKeyEditModeButton"] 71 22
Set-FixedSize $widgetMap["originalKeyRecordCopyButton"] 81 22
Set-FixedSize $widgetMap["pushLevelSpinBox"] 61 22

foreach ($btn in @("oriList_SelectKeyboardButton", "oriList_SelectMouseButton", "oriList_SelectGamepadButton", "oriList_SelectFunctionButton",
                   "mapList_SelectKeyboardButton", "mapList_SelectMouseButton", "mapList_SelectGamepadButton", "mapList_SelectFunctionButton")) {
    Set-FixedSize $widgetMap[$btn] 22 22
}

# Construct the new centralwidget XML
$cwXml = [xml]@'
<widget class="QWidget" name="centralwidget">
 <layout class="QVBoxLayout" name="centralVerticalLayout" stretch="0,1,0">
  <property name="leftMargin">
   <number>6</number>
  </property>
  <property name="topMargin">
   <number>4</number>
  </property>
  <property name="rightMargin">
   <number>6</number>
  </property>
  <property name="bottomMargin">
   <number>6</number>
  </property>
  <property name="spacing">
   <number>4</number>
  </property>
  <item>
   <widget class="Line" name="menuBarSeparatorLine">
    <property name="orientation">
     <enum>Qt::Orientation::Horizontal</enum>
    </property>
   </widget>
  </item>
  <item>
   <widget class="QSplitter" name="mainTableSplitter">
    <property name="orientation">
     <enum>Qt::Orientation::Horizontal</enum>
    </property>
    <property name="childrenCollapsible">
     <bool>false</bool>
    </property>
   </widget>
  </item>
  <item>
   <widget class="QWidget" name="bottomContainerWidget">
    <layout class="QHBoxLayout" name="bottomHorizontalLayout" stretch="1,1">
     <property name="leftMargin">
      <number>0</number>
     </property>
     <property name="topMargin">
      <number>0</number>
     </property>
     <property name="rightMargin">
      <number>0</number>
     </property>
     <property name="bottomMargin">
      <number>0</number>
     </property>
     <property name="spacing">
      <number>8</number>
     </property>
     <item>
      <widget class="QWidget" name="leftPanelWidget">
       <layout class="QVBoxLayout" name="leftPanelVerticalLayout">
        <property name="leftMargin">
         <number>0</number>
        </property>
        <property name="topMargin">
         <number>0</number>
        </property>
        <property name="rightMargin">
         <number>0</number>
        </property>
        <property name="bottomMargin">
         <number>0</number>
        </property>
        <property name="spacing">
         <number>4</number>
        </property>
        <item>
         <layout class="QHBoxLayout" name="settingNameRowLayout" stretch="0,1,0">
          <property name="spacing">
           <number>4</number>
          </property>
         </layout>
        </item>
        <item>
         <layout class="QHBoxLayout" name="settingSelectRowLayout" stretch="1,0">
          <property name="spacing">
           <number>4</number>
          </property>
         </layout>
        </item>
       </layout>
      </widget>
     </item>
     <item>
      <widget class="QWidget" name="rightPanelWidget">
       <layout class="QVBoxLayout" name="rightPanelVerticalLayout">
        <property name="leftMargin">
         <number>0</number>
        </property>
        <property name="topMargin">
         <number>0</number>
        </property>
        <property name="rightMargin">
         <number>0</number>
        </property>
        <property name="bottomMargin">
         <number>0</number>
        </property>
        <property name="spacing">
         <number>4</number>
        </property>
        <item>
         <layout class="QHBoxLayout" name="sourceRowLayout">
          <property name="spacing">
           <number>4</number>
          </property>
         </layout>
        </item>
        <item>
         <layout class="QGridLayout" name="keyEditGridLayout" columnstretch="0,2,0,2,0,0">
          <property name="horizontalSpacing">
           <number>4</number>
          </property>
          <property name="verticalSpacing">
           <number>4</number>
          </property>
         </layout>
        </item>
        <item>
         <layout class="QHBoxLayout" name="actionRowLayout">
          <property name="spacing">
           <number>4</number>
          </property>
         </layout>
        </item>
       </layout>
      </widget>
     </item>
    </layout>
   </widget>
  </item>
 </layout>
</widget>
'@

# Attach splitter children
$splitterNode = $cwXml.SelectSingleNode("//widget[@name='mainTableSplitter']")
$importedProcTable = $cwXml.ImportNode($widgetMap["processinfoTable"], $true)
$importedTabWidget = $cwXml.ImportNode($widgetMap["keyMappingTabWidget"], $true)
[void]$splitterNode.AppendChild($importedProcTable)
[void]$splitterNode.AppendChild($importedTabWidget)

# Attach left panel: settingNameRowLayout
$nameRow = $cwXml.SelectSingleNode("//layout[@name='settingNameRowLayout']")
function Add-LayoutItemWidget($layoutNode, $widgetNode) {
    $item = $cwXml.CreateElement("item")
    $imported = $cwXml.ImportNode($widgetNode, $true)
    [void]$item.AppendChild($imported)
    [void]$layoutNode.AppendChild($item)
}
Add-LayoutItemWidget $nameRow $widgetMap["settingNameLabel"]
Add-LayoutItemWidget $nameRow $widgetMap["settingNameLineEdit"]
Add-LayoutItemWidget $nameRow $widgetMap["backupSettingButton"]

# Attach left panel: settingSelectRowLayout
$selectRow = $cwXml.SelectSingleNode("//layout[@name='settingSelectRowLayout']")
Add-LayoutItemWidget $selectRow $widgetMap["settingselectComboBox"]
Add-LayoutItemWidget $selectRow $widgetMap["savemaplistButton"]

# Modernize windowinfo tab inside settingTabWidget
$tabWindowInfo = $widgetMap["settingTabWidget"].SelectSingleNode("./widget[@name='windowinfo']")
if ($tabWindowInfo) {
    $winWidgets = @{}
    $winChildList = @()
    foreach ($w in $tabWindowInfo.SelectNodes("./widget")) {
        $winChildList += $w
    }
    foreach ($w in $winChildList) {
        $winWidgets[$w.name] = $w
        Clear-Geometry $w
        [void]$tabWindowInfo.RemoveChild($w)
    }

    # Set fixed sizes for windowinfo buttons
    Set-FixedSize $winWidgets["checkProcessComboBox"] 80 21
    Set-FixedSize $winWidgets["checkWindowTitleComboBox"] 80 21
    Set-FixedSize $winWidgets["checkClassNameComboBox"] 80 21
    Set-FixedSize $winWidgets["checkDisplayModeComboBox"] 80 21
    Set-FixedSize $winWidgets["restoreProcessPathButton"] 41 20
    Set-FixedSize $winWidgets["selectSettingCustomIconButton"] 151 21
    Set-FixedSize $winWidgets["ignoreRulesListButton"] 151 22
    Set-FixedSize $winWidgets["iconLabel"] 72 72

    $winLayoutXml = [xml]@'
<layout class="QVBoxLayout" name="windowInfoVerticalLayout">
 <property name="leftMargin"><number>6</number></property>
 <property name="topMargin"><number>6</number></property>
 <property name="rightMargin"><number>6</number></property>
 <property name="bottomMargin"><number>6</number></property>
 <property name="spacing"><number>4</number></property>
 <item>
  <layout class="QGridLayout" name="windowInfoGridLayout" columnstretch="0,0,1,0">
   <property name="horizontalSpacing"><number>4</number></property>
   <property name="verticalSpacing"><number>4</number></property>
  </layout>
 </item>
 <item>
  <layout class="QHBoxLayout" name="iconAndButtonsRowLayout">
   <property name="spacing"><number>8</number></property>
  </layout>
 </item>
 <item>
  <spacer name="spacer_wininfo_v">
   <property name="orientation"><enum>Qt::Orientation::Vertical</enum></property>
   <property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
   <property name="sizeHint"><size><width>20</width><height>10</height></size></property>
  </spacer>
 </item>
</layout>
'@
    $winLayout = $doc.ImportNode($winLayoutXml.DocumentElement, $true)
    [void]$tabWindowInfo.AppendChild($winLayout)

    $winGrid = $tabWindowInfo.SelectSingleNode(".//layout[@name='windowInfoGridLayout']")
    
    function Add-DocGridItemWidget($gridNode, $widgetNode, $row, $col, $rowSpan = 1, $colSpan = 1) {
        $item = $doc.CreateElement("item")
        $item.SetAttribute("row", "$row")
        $item.SetAttribute("column", "$col")
        if ($rowSpan -gt 1) { $item.SetAttribute("rowspan", "$rowSpan") }
        if ($colSpan -gt 1) { $item.SetAttribute("colspan", "$colSpan") }
        [void]$item.AppendChild($widgetNode)
        [void]$gridNode.AppendChild($item)
    }

    # Row 0
    Add-DocGridItemWidget $winGrid $winWidgets["processLabel"] 0 0
    Add-DocGridItemWidget $winGrid $winWidgets["checkProcessComboBox"] 0 1
    Add-DocGridItemWidget $winGrid $winWidgets["processLineEdit"] 0 2
    Add-DocGridItemWidget $winGrid $winWidgets["restoreProcessPathButton"] 0 3

    # Row 1
    Add-DocGridItemWidget $winGrid $winWidgets["windowTitleLabel"] 1 0
    Add-DocGridItemWidget $winGrid $winWidgets["checkWindowTitleComboBox"] 1 1
    Add-DocGridItemWidget $winGrid $winWidgets["windowTitleLineEdit"] 1 2 1 2

    # Row 2
    Add-DocGridItemWidget $winGrid $winWidgets["classNameLabel"] 2 0
    Add-DocGridItemWidget $winGrid $winWidgets["checkClassNameComboBox"] 2 1
    Add-DocGridItemWidget $winGrid $winWidgets["classNameLineEdit"] 2 2 1 2

    # Row 3
    Add-DocGridItemWidget $winGrid $winWidgets["displayModeLabel"] 3 0
    Add-DocGridItemWidget $winGrid $winWidgets["checkDisplayModeComboBox"] 3 1

    # Row 3 sub-layout for description
    $itemDesc = $doc.CreateElement("item")
    $itemDesc.SetAttribute("row", "3")
    $itemDesc.SetAttribute("column", "2")
    $itemDesc.SetAttribute("colspan", "2")
    $descH = $doc.CreateElement("layout")
    $descH.SetAttribute("class", "QHBoxLayout")
    $descH.SetAttribute("name", "descriptionSubLayout")
    $descH.SetAttribute("stretch", "0,1")
    $descH.InnerXml = '<property name="spacing"><number>4</number></property>'
    $it1 = $doc.CreateElement("item"); [void]$it1.AppendChild($winWidgets["descriptionLabel"]); [void]$descH.AppendChild($it1)
    $it2 = $doc.CreateElement("item"); [void]$it2.AppendChild($winWidgets["descriptionLineEdit"]); [void]$descH.AppendChild($it2)
    [void]$itemDesc.AppendChild($descH)
    [void]$winGrid.AppendChild($itemDesc)

    # Row 4: icon and buttons
    $iconRow = $tabWindowInfo.SelectSingleNode(".//layout[@name='iconAndButtonsRowLayout']")
    $itIcon = $doc.CreateElement("item"); [void]$itIcon.AppendChild($winWidgets["iconLabel"]); [void]$iconRow.AppendChild($itIcon)
    $itBtn1 = $doc.CreateElement("item"); [void]$itBtn1.AppendChild($winWidgets["selectSettingCustomIconButton"]); [void]$iconRow.AppendChild($itBtn1)
    $itBtn2 = $doc.CreateElement("item"); [void]$itBtn2.AppendChild($winWidgets["ignoreRulesListButton"]); [void]$iconRow.AppendChild($itBtn2)
    
    $itSp = $doc.CreateElement("item")
    $sp = $doc.CreateElement("spacer")
    $sp.SetAttribute("name", "spacer_icon_h")
    $sp.InnerXml = '<property name="orientation"><enum>Qt::Orientation::Horizontal</enum></property><property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property><property name="sizeHint"><size><width>10</width><height>20</height></size></property>'
    [void]$itSp.AppendChild($sp)
    [void]$iconRow.AppendChild($itSp)
}

function Add-HBoxWidget($rowNode, $wNode) {
    $item = $doc.CreateElement("item")
    [void]$item.AppendChild($wNode)
    [void]$rowNode.AppendChild($item)
}

# Modernize general tab inside settingTabWidget
$tabGeneral = $widgetMap["settingTabWidget"].SelectSingleNode("./widget[@name='general']")
if ($tabGeneral) {
    $genWidgets = @{}
    $genChildList = @()
    foreach ($w in $tabGeneral.SelectNodes("./widget")) { $genChildList += $w }
    foreach ($w in $genChildList) {
        $genWidgets[$w.name] = $w
        Clear-Geometry $w
        [void]$tabGeneral.RemoveChild($w)
    }

    Set-FixedSize $genWidgets["languageComboBox"] 81 21
    Set-FixedSize $genWidgets["notificationComboBox"] 81 21
    Set-FixedSize $genWidgets["updateSiteComboBox"] 81 21
    Set-FixedSize $genWidgets["scaleComboBox"] 81 21
    Set-FixedSize $genWidgets["themeComboBox"] 81 21

    Set-FixedSize $genWidgets["selectTrayIconButton"] 111 21
    Set-FixedSize $genWidgets["notificationAdvancedSettingButton"] 111 21
    Set-FixedSize $genWidgets["checkUpdateButton"] 111 21
    Set-FixedSize $genWidgets["generalAdvancedButton"] 111 21

    $genLayoutXml = [xml]@'
<layout class="QVBoxLayout" name="generalVerticalLayout">
 <property name="leftMargin"><number>6</number></property>
 <property name="topMargin"><number>6</number></property>
 <property name="rightMargin"><number>6</number></property>
 <property name="bottomMargin"><number>6</number></property>
 <property name="spacing"><number>4</number></property>
 <item>
  <layout class="QGridLayout" name="generalGridLayout" columnstretch="1,0,0,0">
   <property name="horizontalSpacing"><number>6</number></property>
   <property name="verticalSpacing"><number>4</number></property>
  </layout>
 </item>
 <item>
  <spacer name="spacer_gen_v">
   <property name="orientation"><enum>Qt::Orientation::Vertical</enum></property>
   <property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
   <property name="sizeHint"><size><width>20</width><height>10</height></size></property>
  </spacer>
 </item>
</layout>
'@
    $genLayout = $doc.ImportNode($genLayoutXml.DocumentElement, $true)
    [void]$tabGeneral.AppendChild($genLayout)
    $genGrid = $tabGeneral.SelectSingleNode(".//layout[@name='generalGridLayout']")

    $itemSw = $doc.CreateElement("item")
    $itemSw.SetAttribute("row", "0"); $itemSw.SetAttribute("column", "0")
    $swH = $doc.CreateElement("layout"); $swH.SetAttribute("class", "QHBoxLayout"); $swH.SetAttribute("name", "switchKeySubLayout")
    $swH.InnerXml = '<property name="spacing"><number>4</number></property>'
    $itSw1 = $doc.CreateElement("item"); [void]$itSw1.AppendChild($genWidgets["windowswitchkeyLabel"]); [void]$swH.AppendChild($itSw1)
    $itSw2 = $doc.CreateElement("item"); [void]$itSw2.AppendChild($genWidgets["windowswitchkeyLineEdit"]); [void]$swH.AppendChild($itSw2)
    [void]$itemSw.AppendChild($swH)
    [void]$genGrid.AppendChild($itemSw)

    Add-DocGridItemWidget $genGrid $genWidgets["languageLabel"] 0 1
    Add-DocGridItemWidget $genGrid $genWidgets["languageComboBox"] 0 2
    Add-DocGridItemWidget $genGrid $genWidgets["selectTrayIconButton"] 0 3

    Add-DocGridItemWidget $genGrid $genWidgets["autoStartupCheckBox"] 1 0
    Add-DocGridItemWidget $genGrid $genWidgets["notificationLabel"] 1 1
    Add-DocGridItemWidget $genGrid $genWidgets["notificationComboBox"] 1 2
    Add-DocGridItemWidget $genGrid $genWidgets["notificationAdvancedSettingButton"] 1 3

    Add-DocGridItemWidget $genGrid $genWidgets["startupMinimizedCheckBox"] 2 0
    Add-DocGridItemWidget $genGrid $genWidgets["updateSiteLabel"] 2 1
    Add-DocGridItemWidget $genGrid $genWidgets["updateSiteComboBox"] 2 2
    Add-DocGridItemWidget $genGrid $genWidgets["checkUpdateButton"] 2 3

    Add-DocGridItemWidget $genGrid $genWidgets["startupAutoMonitoringCheckBox"] 3 0
    Add-DocGridItemWidget $genGrid $genWidgets["scaleLabel"] 3 1
    Add-DocGridItemWidget $genGrid $genWidgets["scaleComboBox"] 3 2
    Add-DocGridItemWidget $genGrid $genWidgets["generalAdvancedButton"] 3 3

    Add-DocGridItemWidget $genGrid $genWidgets["closeToSystemTrayCheckBox"] 4 0
    Add-DocGridItemWidget $genGrid $genWidgets["themeLabel"] 4 1
    Add-DocGridItemWidget $genGrid $genWidgets["themeComboBox"] 4 2
}

# Modernize mapping tab inside settingTabWidget
$tabMapping = $widgetMap["settingTabWidget"].SelectSingleNode("./widget[@name='mapping']")
if ($tabMapping) {
    $mapWidgets = @{}
    $mapChildList = @()
    foreach ($w in $tabMapping.SelectNodes("./widget")) { $mapChildList += $w }
    foreach ($w in $mapChildList) {
        $mapWidgets[$w.name] = $w
        Clear-Geometry $w
        [void]$tabMapping.RemoveChild($w)
    }

    Set-FixedSize $mapWidgets["mappingAdvancedSettingButton"] 141 22
    Set-FixedSize $mapWidgets["mappingMacroListButton"] 141 22
    Set-FixedSize $mapWidgets["vButtonPanelSetupButton"] 141 22
    Set-FixedSize $mapWidgets["installFakerInputButton"] 181 21
    Set-FixedSize $mapWidgets["enableSystemFilterKeyButton"] 191 21

    $mapLayoutXml = [xml]@'
<layout class="QVBoxLayout" name="mappingVerticalLayout">
 <property name="leftMargin"><number>6</number></property>
 <property name="topMargin"><number>6</number></property>
 <property name="rightMargin"><number>6</number></property>
 <property name="bottomMargin"><number>6</number></property>
 <property name="spacing"><number>4</number></property>
 <item>
  <layout class="QGridLayout" name="mappingGridLayout" columnstretch="1,1">
   <property name="horizontalSpacing"><number>10</number></property>
   <property name="verticalSpacing"><number>4</number></property>
  </layout>
 </item>
 <item>
  <spacer name="spacer_map_v">
   <property name="orientation"><enum>Qt::Orientation::Vertical</enum></property>
   <property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
   <property name="sizeHint"><size><width>20</width><height>10</height></size></property>
  </spacer>
 </item>
</layout>
'@
    $mapLayout = $doc.ImportNode($mapLayoutXml.DocumentElement, $true)
    [void]$tabMapping.AppendChild($mapLayout)
    $mapGrid = $tabMapping.SelectSingleNode(".//layout[@name='mappingGridLayout']")

    $itemM0 = $doc.CreateElement("item"); $itemM0.SetAttribute("row", "0"); $itemM0.SetAttribute("column", "0")
    $m0H = $doc.CreateElement("layout"); $m0H.SetAttribute("class", "QHBoxLayout"); $m0H.SetAttribute("name", "mapStartSubLayout")
    $m0H.InnerXml = '<property name="spacing"><number>4</number></property>'
    $itM0_1 = $doc.CreateElement("item"); [void]$itM0_1.AppendChild($mapWidgets["mappingStartKeyLabel"]); [void]$m0H.AppendChild($itM0_1)
    $itM0_2 = $doc.CreateElement("item"); [void]$itM0_2.AppendChild($mapWidgets["mappingStartKeyLineEdit"]); [void]$m0H.AppendChild($itM0_2)
    [void]$itemM0.AppendChild($m0H)
    [void]$mapGrid.AppendChild($itemM0)
    Add-DocGridItemWidget $mapGrid $mapWidgets["autoStartMappingCheckBox"] 0 1

    $itemM1 = $doc.CreateElement("item"); $itemM1.SetAttribute("row", "1"); $itemM1.SetAttribute("column", "0")
    $m1H = $doc.CreateElement("layout"); $m1H.SetAttribute("class", "QHBoxLayout"); $m1H.SetAttribute("name", "mapStopSubLayout")
    $m1H.InnerXml = '<property name="spacing"><number>4</number></property>'
    $itM1_1 = $doc.CreateElement("item"); [void]$itM1_1.AppendChild($mapWidgets["mappingStopKeyLabel"]); [void]$m1H.AppendChild($itM1_1)
    $itM1_2 = $doc.CreateElement("item"); [void]$itM1_2.AppendChild($mapWidgets["mappingStopKeyLineEdit"]); [void]$m1H.AppendChild($itM1_2)
    [void]$itemM1.AppendChild($m1H)
    [void]$mapGrid.AppendChild($itemM1)
    Add-DocGridItemWidget $mapGrid $mapWidgets["enableSystemFilterKeyButton"] 1 1

    Add-DocGridItemWidget $mapGrid $mapWidgets["mappingAdvancedSettingButton"] 2 0
    Add-DocGridItemWidget $mapGrid $mapWidgets["sendToSameTitleWindowsCheckBox"] 2 1

    Add-DocGridItemWidget $mapGrid $mapWidgets["mappingMacroListButton"] 3 0
    Add-DocGridItemWidget $mapGrid $mapWidgets["FakerInputStatusLabel"] 3 1

    Add-DocGridItemWidget $mapGrid $mapWidgets["vButtonPanelSetupButton"] 4 0
    Add-DocGridItemWidget $mapGrid $mapWidgets["installFakerInputButton"] 4 1
}

# Modernize virtualgamepad tab inside settingTabWidget
$tabVPad = $widgetMap["settingTabWidget"].SelectSingleNode("./widget[@name='virtualgamepad']")
if ($tabVPad) {
    $vpadWidgets = @{}
    $vpadChildList = @()
    foreach ($w in $tabVPad.SelectNodes("./widget")) { $vpadChildList += $w }
    foreach ($w in $vpadChildList) {
        $vpadWidgets[$w.name] = $w
        Clear-Geometry $w
        [void]$tabVPad.RemoveChild($w)
    }

    Set-FixedSize $vpadWidgets["installViGEmBusButton"] 91 21

    $vpadLayoutXml = [xml]@'
<layout class="QVBoxLayout" name="vpadVerticalLayout">
 <property name="leftMargin"><number>6</number></property>
 <property name="topMargin"><number>6</number></property>
 <property name="rightMargin"><number>6</number></property>
 <property name="bottomMargin"><number>6</number></property>
 <property name="spacing"><number>4</number></property>
 <item>
  <layout class="QHBoxLayout" name="vpadRow0Layout">
   <property name="spacing"><number>4</number></property>
  </layout>
 </item>
 <item>
  <layout class="QHBoxLayout" name="vpadRow1Layout">
   <property name="spacing"><number>6</number></property>
  </layout>
 </item>
 <item>
  <layout class="QHBoxLayout" name="vpadRow2Layout">
   <property name="spacing"><number>6</number></property>
  </layout>
 </item>
 <item>
  <layout class="QHBoxLayout" name="vpadRow3Layout">
   <property name="spacing"><number>6</number></property>
  </layout>
 </item>
 <item>
  <spacer name="spacer_vpad_v">
   <property name="orientation"><enum>Qt::Orientation::Vertical</enum></property>
   <property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
   <property name="sizeHint"><size><width>20</width><height>10</height></size></property>
  </spacer>
 </item>
</layout>
'@
    $vpadLayout = $doc.ImportNode($vpadLayoutXml.DocumentElement, $true)
    [void]$tabVPad.AppendChild($vpadLayout)

    $r0 = $tabVPad.SelectSingleNode(".//layout[@name='vpadRow0Layout']")
    Add-HBoxWidget $r0 $vpadWidgets["enableVirtualJoystickCheckBox"]
    Add-HBoxWidget $r0 $vpadWidgets["virtualGamepadTypeComboBox"]
    Add-HBoxWidget $r0 $vpadWidgets["virtualGamepadNumberSpinBox"]
    Add-HBoxWidget $r0 $vpadWidgets["virtualGamepadListComboBox"]
    Add-HBoxWidget $r0 $vpadWidgets["ViGEmBusStatusLabel"]
    Add-HBoxWidget $r0 $vpadWidgets["installViGEmBusButton"]

    $r1 = $tabVPad.SelectSingleNode(".//layout[@name='vpadRow1Layout']")
    Add-HBoxWidget $r1 $vpadWidgets["directModeCheckBox"]
    Add-HBoxWidget $r1 $vpadWidgets["lockCursorCheckBox"]
    Add-HBoxWidget $r1 $vpadWidgets["vJoyRecenterLabel"]
    Add-HBoxWidget $r1 $vpadWidgets["vJoyRecenterSpinBox"]

    $r2 = $tabVPad.SelectSingleNode(".//layout[@name='vpadRow2Layout']")
    Add-HBoxWidget $r2 $vpadWidgets["vJoyXSensLabel"]
    Add-HBoxWidget $r2 $vpadWidgets["vJoyXSensSpinBox"]
    Add-HBoxWidget $r2 $vpadWidgets["vJoyInvertXCheckBox"]

    $r3 = $tabVPad.SelectSingleNode(".//layout[@name='vpadRow3Layout']")
    Add-HBoxWidget $r3 $vpadWidgets["vJoyYSensLabel"]
    Add-HBoxWidget $r3 $vpadWidgets["vJoyYSensSpinBox"]
    Add-HBoxWidget $r3 $vpadWidgets["vJoyInvertYCheckBox"]
}

# Modernize gyro2mouse tab inside settingTabWidget
$tabGyro = $widgetMap["settingTabWidget"].SelectSingleNode("./widget[@name='gyro2mouse']")
if ($tabGyro) {
    $gyroWidgets = @{}
    $gyroChildList = @()
    foreach ($w in $tabGyro.SelectNodes("./widget")) { $gyroChildList += $w }
    foreach ($w in $gyroChildList) {
        $gyroWidgets[$w.name] = $w
        Clear-Geometry $w
        [void]$tabGyro.RemoveChild($w)
    }

    Set-FixedSize $gyroWidgets["Gyro2MouseAdvancedSettingButton"] 101 22

    $gyroLayoutXml = [xml]@'
<layout class="QVBoxLayout" name="gyroVerticalLayout">
 <property name="leftMargin"><number>6</number></property>
 <property name="topMargin"><number>6</number></property>
 <property name="rightMargin"><number>6</number></property>
 <property name="bottomMargin"><number>6</number></property>
 <property name="spacing"><number>4</number></property>
 <item>
  <layout class="QGridLayout" name="gyroGridLayout">
   <property name="horizontalSpacing"><number>6</number></property>
   <property name="verticalSpacing"><number>4</number></property>
  </layout>
 </item>
 <item>
  <spacer name="spacer_gyro_v">
   <property name="orientation"><enum>Qt::Orientation::Vertical</enum></property>
   <property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
   <property name="sizeHint"><size><width>20</width><height>10</height></size></property>
  </spacer>
 </item>
</layout>
'@
    $gyroLayout = $doc.ImportNode($gyroLayoutXml.DocumentElement, $true)
    [void]$tabGyro.AppendChild($gyroLayout)
    $gyroGrid = $tabGyro.SelectSingleNode(".//layout[@name='gyroGridLayout']")

    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseXSpeedLabel"] 0 0
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseXSpeedSpinBox"] 0 1
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMinXSensLabel"] 0 2
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMinXSensSpinBox"] 0 3
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMaxXSensLabel"] 0 4
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMaxXSensSpinBox"] 0 5

    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseYSpeedLabel"] 1 0
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseYSpeedSpinBox"] 1 1
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMinYSensLabel"] 1 2
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMinYSensSpinBox"] 1 3
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMaxYSensLabel"] 1 4
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMaxYSensSpinBox"] 1 5

    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseAdvancedSettingButton"] 2 0 1 2
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMinThresholdLabel"] 2 2
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMinThresholdSpinBox"] 2 3
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMaxThresholdLabel"] 2 4
    Add-DocGridItemWidget $gyroGrid $gyroWidgets["Gyro2MouseMaxThresholdSpinBox"] 2 5
}

# Modernize multiinput tab inside settingTabWidget
$tabMulti = $widgetMap["settingTabWidget"].SelectSingleNode("./widget[@name='multiinput']")
if ($tabMulti) {
    $multiWidgets = @{}
    $multiChildList = @()
    foreach ($w in $tabMulti.SelectNodes("./widget")) { $multiChildList += $w }
    foreach ($w in $multiChildList) {
        $multiWidgets[$w.name] = $w
        Clear-Geometry $w
        [void]$tabMulti.RemoveChild($w)
    }

    Set-FixedSize $multiWidgets["multiInputDeviceListButton"] 101 22
    Set-FixedSize $multiWidgets["installInterceptionButton"] 91 21

    $multiLayoutXml = [xml]@'
<layout class="QVBoxLayout" name="multiVerticalLayout">
 <property name="leftMargin"><number>6</number></property>
 <property name="topMargin"><number>6</number></property>
 <property name="rightMargin"><number>6</number></property>
 <property name="bottomMargin"><number>6</number></property>
 <property name="spacing"><number>4</number></property>
 <item>
  <layout class="QHBoxLayout" name="multiRow0Layout">
   <property name="spacing"><number>6</number></property>
  </layout>
 </item>
 <item>
  <layout class="QHBoxLayout" name="multiRow1Layout">
   <property name="spacing"><number>6</number></property>
  </layout>
 </item>
 <item>
  <spacer name="spacer_multi_v">
   <property name="orientation"><enum>Qt::Orientation::Vertical</enum></property>
   <property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
   <property name="sizeHint"><size><width>20</width><height>10</height></size></property>
  </spacer>
 </item>
</layout>
'@
    $multiLayout = $doc.ImportNode($multiLayoutXml.DocumentElement, $true)
    [void]$tabMulti.AppendChild($multiLayout)

    $mr0 = $tabMulti.SelectSingleNode(".//layout[@name='multiRow0Layout']")
    Add-HBoxWidget $mr0 $multiWidgets["multiInputEnableCheckBox"]
    Add-HBoxWidget $mr0 $multiWidgets["multiInputDeviceListButton"]
    Add-HBoxWidget $mr0 $multiWidgets["multiInputStatusLabel"]
    Add-HBoxWidget $mr0 $multiWidgets["installInterceptionButton"]

    $mr1 = $tabMulti.SelectSingleNode(".//layout[@name='multiRow1Layout']")
    Add-HBoxWidget $mr1 $multiWidgets["filterKeysCheckBox"]
}

# Modernize forza tab inside settingTabWidget
$tabForza = $widgetMap["settingTabWidget"].SelectSingleNode("./widget[@name='forza']")
if ($tabForza) {
    $forzaWidgets = @{}
    $forzaChildList = @()
    foreach ($w in $tabForza.SelectNodes("./widget")) { $forzaChildList += $w }
    foreach ($w in $forzaChildList) {
        $forzaWidgets[$w.name] = $w
        Clear-Geometry $w
        [void]$tabForza.RemoveChild($w)
    }

    $forzaLayoutXml = [xml]@'
<layout class="QVBoxLayout" name="forzaVerticalLayout">
 <property name="leftMargin"><number>6</number></property>
 <property name="topMargin"><number>6</number></property>
 <property name="rightMargin"><number>6</number></property>
 <property name="bottomMargin"><number>6</number></property>
 <property name="spacing"><number>4</number></property>
 <item>
  <layout class="QGridLayout" name="forzaGridLayout" columnstretch="0,1">
   <property name="horizontalSpacing"><number>6</number></property>
   <property name="verticalSpacing"><number>4</number></property>
  </layout>
 </item>
 <item>
  <spacer name="spacer_forza_v">
   <property name="orientation"><enum>Qt::Orientation::Vertical</enum></property>
   <property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
   <property name="sizeHint"><size><width>20</width><height>10</height></size></property>
  </spacer>
 </item>
</layout>
'@
    $forzaLayout = $doc.ImportNode($forzaLayoutXml.DocumentElement, $true)
    [void]$tabForza.AppendChild($forzaLayout)
    $forzaGrid = $tabForza.SelectSingleNode(".//layout[@name='forzaGridLayout']")

    Add-DocGridItemWidget $forzaGrid $forzaWidgets["brakeThresholdLabel"] 0 0
    Add-DocGridItemWidget $forzaGrid $forzaWidgets["brakeThresholdDoubleSpinBox"] 0 1
    Add-DocGridItemWidget $forzaGrid $forzaWidgets["accelThresholdLabel"] 1 0
    Add-DocGridItemWidget $forzaGrid $forzaWidgets["accelThresholdDoubleSpinBox"] 1 1
    Add-DocGridItemWidget $forzaGrid $forzaWidgets["dataPortLabel"] 2 0
    Add-DocGridItemWidget $forzaGrid $forzaWidgets["dataPortSpinBox"] 2 1
}

# Attach left panel: settingTabWidget
$leftPanel = $cwXml.SelectSingleNode("//widget[@name='leftPanelWidget']/layout")
Add-LayoutItemWidget $leftPanel $widgetMap["settingTabWidget"]

# Attach right panel: sourceRowLayout
$srcRow = $cwXml.SelectSingleNode("//layout[@name='sourceRowLayout']")
function Add-Spacer($layoutNode, $w, $h) {
    $item = $cwXml.CreateElement("item")
    $sp = $cwXml.CreateElement("spacer")
    $sp.SetAttribute("name", "spacer_" + [System.Guid]::NewGuid().ToString("N").Substring(0, 8))
    $sp.InnerXml = @"
<property name="orientation"><enum>Qt::Orientation::Horizontal</enum></property>
<property name="sizeType"><enum>QSizePolicy::Policy::Expanding</enum></property>
<property name="sizeHint"><size><width>$w</width><height>$h</height></size></property>
"@
    [void]$item.AppendChild($sp)
    [void]$layoutNode.AppendChild($item)
}

foreach ($btn in @("oriList_SelectKeyboardButton", "oriList_SelectMouseButton", "oriList_SelectGamepadButton", "oriList_SelectFunctionButton")) {
    Add-LayoutItemWidget $srcRow $widgetMap[$btn]
}
Add-Spacer $srcRow 10 20
foreach ($btn in @("mapList_SelectKeyboardButton", "mapList_SelectMouseButton", "mapList_SelectGamepadButton", "mapList_SelectFunctionButton")) {
    Add-LayoutItemWidget $srcRow $widgetMap[$btn]
}
Add-Spacer $srcRow 10 20
Add-LayoutItemWidget $srcRow $widgetMap["addmapdataButton"]

# Attach right panel: keyEditGridLayout
$grid = $cwXml.SelectSingleNode("//layout[@name='keyEditGridLayout']")
function Add-GridItemWidget($gridNode, $widgetNode, $row, $col, $rowSpan = 1, $colSpan = 1) {
    $item = $cwXml.CreateElement("item")
    $item.SetAttribute("row", "$row")
    $item.SetAttribute("column", "$col")
    if ($rowSpan -gt 1) { $item.SetAttribute("rowspan", "$rowSpan") }
    if ($colSpan -gt 1) { $item.SetAttribute("colspan", "$colSpan") }
    $imported = $cwXml.ImportNode($widgetNode, $true)
    [void]$item.AppendChild($imported)
    [void]$gridNode.AppendChild($item)
}

# Row 0: orikeyLabel (0,0), orikeyComboBox (0,1), mapkeyLabel (0,2), mapkeyComboBox (0,3, 1, 3)
Add-GridItemWidget $grid $widgetMap["orikeyLabel"] 0 0
Add-GridItemWidget $grid $widgetMap["orikeyComboBox"] 0 1
Add-GridItemWidget $grid $widgetMap["mapkeyLabel"] 0 2
Add-GridItemWidget $grid $widgetMap["mapkeyComboBox"] 0 3 1 3

# Row 1: orikeyRecordLabel (1,0), originalKeyRecordLineEdit (1,1, 1, 3), originalKeyEditModeButton (1,4), originalKeyRecordCopyButton (1,5)
Add-GridItemWidget $grid $widgetMap["orikeyRecordLabel"] 1 0
Add-GridItemWidget $grid $widgetMap["originalKeyRecordLineEdit"] 1 1 1 3
Add-GridItemWidget $grid $widgetMap["originalKeyEditModeButton"] 1 4
Add-GridItemWidget $grid $widgetMap["originalKeyRecordCopyButton"] 1 5

# Row 2: trigger row
Add-GridItemWidget $grid $widgetMap["triggerTypeLabel"] 2 0
# Sub-layout for trigger controls in column 1..2
$itemTrigger = $cwXml.CreateElement("item")
$itemTrigger.SetAttribute("row", "2")
$itemTrigger.SetAttribute("column", "1")
$itemTrigger.SetAttribute("colspan", "2")
$subH = $cwXml.CreateElement("layout")
$subH.SetAttribute("class", "QHBoxLayout")
$subH.SetAttribute("name", "triggerSubLayout")
$subH.InnerXml = '<property name="spacing"><number>4</number></property>'
Add-LayoutItemWidget $subH $widgetMap["keyPressTypeComboBox"]
Add-LayoutItemWidget $subH $widgetMap["pressTimeSpinBox"]
[void]$itemTrigger.AppendChild($subH)
[void]$grid.AppendChild($itemTrigger)

# Wait time in col 3..4
$itemWait = $cwXml.CreateElement("item")
$itemWait.SetAttribute("row", "2")
$itemWait.SetAttribute("column", "3")
$itemWait.SetAttribute("colspan", "2")
$subWait = $cwXml.CreateElement("layout")
$subWait.SetAttribute("class", "QHBoxLayout")
$subWait.SetAttribute("name", "waitSubLayout")
$subWait.InnerXml = '<property name="spacing"><number>4</number></property>'
Add-LayoutItemWidget $subWait $widgetMap["waitTimeLabel"]
Add-LayoutItemWidget $subWait $widgetMap["waitTimeSpinBox"]
[void]$itemWait.AppendChild($subWait)
[void]$grid.AppendChild($itemWait)

# Point in col 5
$itemPoint = $cwXml.CreateElement("item")
$itemPoint.SetAttribute("row", "2")
$itemPoint.SetAttribute("column", "5")
$subPt = $cwXml.CreateElement("layout")
$subPt.SetAttribute("class", "QHBoxLayout")
$subPt.SetAttribute("name", "pointSubLayout")
$subPt.InnerXml = '<property name="spacing"><number>4</number></property>'
Add-LayoutItemWidget $subPt $widgetMap["pointLabel"]
Add-LayoutItemWidget $subPt $widgetMap["pointDisplayLabel"]
[void]$itemPoint.AppendChild($subPt)
[void]$grid.AppendChild($itemPoint)

# Row 3: keyboardSelectLabel (3,0), keyboardSelectComboBox (3,1), pushLevelLabel (3,2), pushLevelSlider (3,3, 1, 2), pushLevelSpinBox (3,5)
Add-GridItemWidget $grid $widgetMap["keyboardSelectLabel"] 3 0
Add-GridItemWidget $grid $widgetMap["keyboardSelectComboBox"] 3 1
Add-GridItemWidget $grid $widgetMap["pushLevelLabel"] 3 2
Add-GridItemWidget $grid $widgetMap["pushLevelSlider"] 3 3 1 2
Add-GridItemWidget $grid $widgetMap["pushLevelSpinBox"] 3 5

# Row 4: mouseSelectLabel (4,0), mouseSelectComboBox (4,1), sendTextLabel (4,2), sendTextPlainTextEdit (4,3, 2, 3)
Add-GridItemWidget $grid $widgetMap["mouseSelectLabel"] 4 0
Add-GridItemWidget $grid $widgetMap["mouseSelectComboBox"] 4 1
Add-GridItemWidget $grid $widgetMap["sendTextLabel"] 4 2
Add-GridItemWidget $grid $widgetMap["sendTextPlainTextEdit"] 4 3 2 3

# Row 5: gamepadSelectLabel (5,0), gamepadSelectComboBox (5,1)
Add-GridItemWidget $grid $widgetMap["gamepadSelectLabel"] 5 0
Add-GridItemWidget $grid $widgetMap["gamepadSelectComboBox"] 5 1

# Action row: Spacer + keymapButton
$actRow = $cwXml.SelectSingleNode("//layout[@name='actionRowLayout']")
Add-Spacer $actRow 10 20
Add-LayoutItemWidget $actRow $widgetMap["keymapButton"]

# Replace centralwidget in $doc
$mainWin = $doc.ui.widget | Where-Object { $_.name -eq "QKeyMapper" }
$importedCw = $doc.ImportNode($cwXml.DocumentElement, $true)
[void]$mainWin.ReplaceChild($importedCw, $cw)

# Save modern UI XML
$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.IndentChars = " "
$settings.Encoding = [System.Text.Encoding]::UTF8
$writer = [System.Xml.XmlWriter]::Create($modernUiPath, $settings)
$doc.Save($writer)
$writer.Dispose()

Write-Host "Modern UI XML generated: $modernUiPath"
