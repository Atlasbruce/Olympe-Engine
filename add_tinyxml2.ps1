# Script to add tinyxml2 to vcxproj
$vcxprojPath = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"
$content = Get-Content $vcxprojPath -Raw

# Find the line with TiledDecoder.cpp and add tinyxml2 after
$searchPattern = '(\s+<ClCompile Include="\.\.\\Source\\TiledLevelLoader\\src\\TiledDecoder\.cpp" />)'

# Create replacement with tinyxml2
$tiledFiles = @'
    <ClCompile Include="..\Source\TiledLevelLoader\src\TiledDecoder.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\third_party\tinyxml2\tinyxml2.cpp" />
'@

# Replace
$newContent = $content -replace $searchPattern, $tiledFiles

# Save
Set-Content $vcxprojPath -Value $newContent -NoNewline -Encoding UTF8

Write-Host "tinyxml2.cpp added successfully"
