# Script to exclude Phase 2 files from build

$vcxproj = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"

# Read the file
[xml]$xml = Get-Content $vcxproj

# Files to exclude
$filesToExclude = @(
    "..\Source\NodeGraphCore\NodeGraphPanel.cpp",
    "..\Source\NodeGraphCore\NodeGraphRenderer.cpp",
    "..\Source\NodeGraphShared\BTGraphDocumentConverter.cpp",
    "..\Source\NodeGraphShared\BTDebugAdapter.cpp",
    "..\Source\NodeGraphCore\Commands\DeleteLinkCommand.cpp"
)

# Find and mark files as excluded
foreach ($itemGroup in $xml.Project.ItemGroup) {
    foreach ($compile in $itemGroup.ClCompile) {
        if ($filesToExclude -contains $compile.Include) {
            Write-Host "Excluding: $($compile.Include)"
            $excludeNode = $xml.CreateElement("ExcludedFromBuild", $xml.DocumentElement.NamespaceURI)
            $excludeNode.InnerText = "true"
            $compile.AppendChild($excludeNode) | Out-Null
        }
    }
}

# Save
$xml.Save((Resolve-Path $vcxproj).Path)

Write-Host "`nDone! Files excluded from build."
