[System.Xml.XmlDocument]$xml = New-Object System.Xml.XmlDocument
$xml.Load("OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj")

# Get the namespace
$ns = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)
$ns.AddNamespace('msbuild', 'http://schemas.microsoft.com/developer/msbuild/2003')

# Find all ItemGroups with ClCompile
$itemGroups = $xml.SelectNodes("//msbuild:ItemGroup[msbuild:ClCompile]", $ns)

# Get the first ItemGroup
$firstItemGroup = $itemGroups[0]

# Create new nodes for our files
$newNodeMathOp = $xml.CreateElement("ClCompile", "http://schemas.microsoft.com/developer/msbuild/2003")
$newNodeMathOp.SetAttribute("Include", "..\Source\BlueprintEditor\MathOpOperand.cpp")
$firstItemGroup.AppendChild($newNodeMathOp) | Out-Null

$newNodePanel = $xml.CreateElement("ClCompile", "http://schemas.microsoft.com/developer/msbuild/2003")
$newNodePanel.SetAttribute("Include", "..\Source\Editor\Panels\MathOpPropertyPanel.cpp")
$firstItemGroup.AppendChild($newNodePanel) | Out-Null

# Save
$xml.Save("OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj")
Write-Host "Added entries to OlympeBlueprintEditor.vcxproj"
