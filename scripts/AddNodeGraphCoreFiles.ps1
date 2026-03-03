# Add missing NodeGraphCore files to project

$vcxproj = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"

[xml]$xml = Get-Content $vcxproj
$ns = $xml.DocumentElement.NamespaceURI

# Find ItemGroup with ClCompile
$itemGroup = $xml.Project.ItemGroup | Where-Object { $_.ClCompile } | Select-Object -First 1

# Add EditorContext.cpp
$newNode = $xml.CreateElement("ClCompile", $ns)
$newNode.SetAttribute("Include", "..\Source\NodeGraphCore\EditorContext.cpp")
$itemGroup.AppendChild($newNode) | Out-Null

# Add NodeGraphPanel.cpp (the real one from NodeGraphCore that exists)
$newNode2 = $xml.CreateElement("ClCompile", $ns)
$newNode2.SetAttribute("Include", "..\Source\NodeGraphCore\NodeGraphPanel.cpp")
$itemGroup.AppendChild($newNode2) | Out-Null

$xml.Save((Resolve-Path $vcxproj).Path)

Write-Host "Added NodeGraphCore files to project:"
Write-Host "  - EditorContext.cpp"
Write-Host "  - NodeGraphPanel.cpp"
