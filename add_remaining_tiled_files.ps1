# Script to add remaining Tiled files to vcxproj
$vcxprojPath = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"
$content = Get-Content $vcxprojPath -Raw

# Find the line with TilesetCache.cpp and add new files after
$searchPattern = '(\s+<ClCompile Include="\.\.\\Source\\TiledLevelLoader\\src\\TilesetCache\.cpp" />)'

# Create replacement with new files
$tiledFiles = @'
    <ClCompile Include="..\Source\TiledLevelLoader\src\TilesetCache.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\src\TilesetParser.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\src\TiledDecoder.cpp" />
'@

# Replace
$newContent = $content -replace $searchPattern, $tiledFiles

# Save
Set-Content $vcxprojPath -Value $newContent -NoNewline -Encoding UTF8

Write-Host "TilesetParser.cpp and TiledDecoder.cpp added successfully"
