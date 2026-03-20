$xml = [System.Xml.XmlDocument]::new()
$xml.Load("OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj")

$ns = [System.Xml.XmlNamespaceManager]::new($xml.NameTable)
$ns.AddNamespace('msbuild', 'http://schemas.microsoft.com/developer/msbuild/2003')

$itemGroups = $xml.SelectNodes("//msbuild:ItemGroup[msbuild:ClCompile]", $ns)
$firstItemGroup = $itemGroups[0]

$newNode1 = $xml.CreateElement("ClCompile", "http://schemas.microsoft.com/developer/msbuild/2003")
$newNode1.SetAttribute("Include", "..\Source\Editor\Panels\GetBBValuePropertyPanel.cpp")
$firstItemGroup.AppendChild($newNode1) | Out-Null

$newNode2 = $xml.CreateElement("ClCompile", "http://schemas.microsoft.com/developer/msbuild/2003")
$newNode2.SetAttribute("Include", "..\Source\Editor\Panels\SetBBValuePropertyPanel.cpp")
$firstItemGroup.AppendChild($newNode2) | Out-Null

$headerGroups = $xml.SelectNodes("//msbuild:ItemGroup[msbuild:ClInclude]", $ns)
$firstHeaderGroup = $headerGroups[0]

$newHeader1 = $xml.CreateElement("ClInclude", "http://schemas.microsoft.com/developer/msbuild/2003")
$newHeader1.SetAttribute("Include", "..\Source\Editor\Panels\GetBBValuePropertyPanel.h")
$firstHeaderGroup.AppendChild($newHeader1) | Out-Null

$newHeader2 = $xml.CreateElement("ClInclude", "http://schemas.microsoft.com/developer/msbuild/2003")
$newHeader2.SetAttribute("Include", "..\Source\Editor\Panels\SetBBValuePropertyPanel.h")
$firstHeaderGroup.AppendChild($newHeader2) | Out-Null

$xml.Save("OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj")
Write-Host "Successfully added GetBBValue and SetBBValue panels to project"
