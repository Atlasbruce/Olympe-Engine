# Script to add ProjectReference to OlympeBlueprintEditor.vcxproj
$vcxprojPath = "C:\Users\Nico\source\repos\Atlasbruce\Olympe-Engine\OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"

# Load the XML
[xml]$xml = Get-Content $vcxprojPath

# Get the root namespace
$ns = @{ msbuild = "http://schemas.microsoft.com/developer/msbuild/2003" }

# Create new ProjectReference ItemGroup
$newItemGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")

$projectRef = $xml.CreateElement("ProjectReference", "http://schemas.microsoft.com/developer/msbuild/2003")
$projectRef.SetAttribute("Include", "..\Olympe Engine.vcxproj")

$projectGuid = $xml.CreateElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003")
$projectGuid.InnerText = "{7e778348-7ae8-44c3-95a4-303274248fe5}"

$refOutputAssembly = $xml.CreateElement("ReferenceOutputAssembly", "http://schemas.microsoft.com/developer/msbuild/2003")
$refOutputAssembly.InnerText = "true"

$projectRef.AppendChild($projectGuid)
$projectRef.AppendChild($refOutputAssembly)
$newItemGroup.AppendChild($projectRef)

# Find PropertyGroup with Label="Globals" and insert before it
$propertyGroup = $xml.SelectSingleNode("//msbuild:PropertyGroup[@Label='Globals']", $ns)
if ($propertyGroup -ne $null) {
    $propertyGroup.ParentNode.InsertBefore($newItemGroup, $propertyGroup)
    $xml.Save($vcxprojPath)
    Write-Host "ProjectReference added successfully!"
} else {
    Write-Host "Could not find PropertyGroup with Label='Globals'"
}
