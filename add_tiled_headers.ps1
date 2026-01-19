# Script to add Tiled header files to vcxproj
$vcxprojPath = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"
$content = Get-Content $vcxprojPath -Raw

# Find the line with resource.h and closing ItemGroup
$searchPattern = '(\s+<ClInclude Include="resource\.h" />\s+</ItemGroup>)'

# Create replacement with new header files
$tiledHeaders = @"
    <ClInclude Include="resource.h" />
    <ClInclude Include="..\Source\TiledLevelLoader\include\TiledLevelLoader.h" />
    <ClInclude Include="..\Source\TiledLevelLoader\include\TiledToOlympe.h" />
    <ClInclude Include="..\Source\TiledLevelLoader\include\ParallaxLayerManager.h" />
    <ClInclude Include="..\Source\TiledLevelLoader\include\IsometricProjection.h" />
    <ClInclude Include="..\Source\TiledLevelLoader\include\TilesetCache.h" />
  </ItemGroup>
"@

# Replace
$newContent = $content -replace $searchPattern, $tiledHeaders

# Save
Set-Content $vcxprojPath -Value $newContent -NoNewline -Encoding UTF8

Write-Host "Tiled header files added successfully to $vcxprojPath"
