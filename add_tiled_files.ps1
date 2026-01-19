# Script to add Tiled files to vcxproj
$vcxprojPath = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"
$content = Get-Content $vcxprojPath -Raw

# Find the line with imnodes.cpp and the closing ItemGroup
$searchPattern = '(\s+<ClCompile Include="\.\.\\Source\\third_party\\imnodes\\imnodes\.cpp" />\s+</ItemGroup>)'

# Create replacement with new files
$tiledFiles = @"
    <ClCompile Include="..\Source\third_party\imnodes\imnodes.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\src\TiledLevelLoader.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\src\TiledToOlympe.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\src\ParallaxLayerManager.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\src\IsometricProjection.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\src\TilesetCache.cpp" />
  </ItemGroup>
"@

# Replace
$newContent = $content -replace $searchPattern, $tiledFiles

# Save
Set-Content $vcxprojPath -Value $newContent -NoNewline -Encoding UTF8

Write-Host "Tiled files added successfully to $vcxprojPath"
