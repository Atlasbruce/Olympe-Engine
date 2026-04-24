# Add missing Framework files to project
$filePath = 'OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj'
$content = [System.IO.File]::ReadAllText($filePath)

$oldText = '    <ClCompile Include="..\Source\BlueprintEditor\Framework\CanvasToolbarRenderer.cpp" />'
$newText = @'
    <ClCompile Include="..\Source\BlueprintEditor\Framework\CanvasToolbarRenderer.cpp" />
    <ClCompile Include="..\Source\BlueprintEditor\Framework\GraphEditorBase.cpp" />
    <ClCompile Include="..\Source\BlueprintEditor\Framework\IEditorToolManager.cpp" />
'@

$newContent = $content.Replace($oldText, $newText)
[System.IO.File]::WriteAllText($filePath, $newContent)
Write-Output "Successfully added Framework files to vcxproj"
