# Simple script to add Placeholder files to project
$filePath = 'OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj'
$content = [System.IO.File]::ReadAllText($filePath)

$oldText = '    <ClCompile Include="..\Source\BlueprintEditor\EntityPrefabEditorPlugin.cpp" />'
$newText = @'
    <ClCompile Include="..\Source\BlueprintEditor\EntityPrefabEditorPlugin.cpp" />
    <ClCompile Include="..\Source\BlueprintEditor\PlaceholderEditor\PlaceholderGraphDocument.cpp" />
    <ClCompile Include="..\Source\BlueprintEditor\PlaceholderEditor\PlaceholderCanvas.cpp" />
    <ClCompile Include="..\Source\BlueprintEditor\PlaceholderEditor\PlaceholderGraphRenderer.cpp" />
'@

$newContent = $content.Replace($oldText, $newText)
[System.IO.File]::WriteAllText($filePath, $newContent)
Write-Output "Successfully added Placeholder files to vcxproj"
