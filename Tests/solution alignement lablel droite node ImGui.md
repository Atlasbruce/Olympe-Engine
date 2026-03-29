Tu peux t’en sortir proprement en ne touchant plus au layout ImGui à l’intérieur de BeginOutputAttribute, et en dessinant le texte par‑dessus avec ImDrawList en te basant sur le rect de l’attribut.
Ça contourne complètement les problèmes de SetCursorPosX, Indent, tables, etc., que tu as déjà constatés.
​

Idée générale
À l’intérieur du BeginOutputAttribute/EndOutputAttribute, tu ne fais qu’un Dummy() pour réserver la hauteur de ligne (pour que le node garde une géométrie correcte).

Juste après cet item, tu récupères son rectangle via ImGui::GetItemRectMin/Max().

À partir de ce rect, tu calcules la position du texte aligné à droite (rect.max.x moins un padding et moins la largeur du texte).

Tu dessines le label avec ImGui::GetWindowDrawList()->AddText(...).

Comme tu ne modifies pas le curseur ImGui ni le layout d’ImNodes, le node reste stable, et comme tu calcules la position à partir du rect courant, le label suit automatiquement quand le node est redimensionné.
​

Étapes concrètes
Supposons que tu as déjà quelque chose du genre :

cpp
ImNodes::BeginOutputAttribute(pinId);
ImGui::TextUnformatted(label.c_str());
ImNodes::EndOutputAttribute();
Remplace‑le par :

cpp
void DrawRightAlignedOutputPin(const char* label, int pinId)
{
    using namespace ImGui;

    ImNodes::BeginOutputAttribute(pinId);

    // 1) On réserve juste la hauteur de ligne, sans texte
    float line_h = GetTextLineHeight();
    Dummy(ImVec2(0.0f, line_h));

    // 2) On récupère le rect de la "ligne de pin"
    ImVec2 rect_min = GetItemRectMin();
    ImVec2 rect_max = GetItemRectMax();

    ImNodes::EndOutputAttribute();

    // 3) Calcul de la position du texte aligné à droite
    ImDrawList* dl = GetWindowDrawList();
    ImVec2 text_size = CalcTextSize(label);

    const auto& styleNodes = ImNodes::GetStyle();       // si dispo
    float pin_radius = styleNodes.PinCircleRadius;      // ou une valeur constante
    float padding = 4.0f;                               // distance texte‑>pin

    float x = rect_max.x - (pin_radius * 2.0f) - padding - text_size.x;
    float y = rect_min.y + (rect_max.y - rect_min.y - text_size.y) * 0.5f;

    // 4) Dessin du texte dans le drawlist de la fenêtre
    ImU32 col = GetColorU32(ImGuiCol_Text);
    dl->AddText(ImVec2(x, y), col, label);
}
Et dans ton code de node :

cpp
DrawRightAlignedOutputPin("Completed", outputPinId);
Pourquoi c’est robuste
Aucune modification du curseur / layout pendant le scope ImNodes → tu évites exactement les plantages de layout que tu as observés avec SetCursorPosX, Indent, tables, etc.
​

Alignement dynamique : le rect de l’item (GetItemRectMin/Max) bouge automatiquement quand le node change de taille, donc le texte reste collé à la bordure droite / au pin, même si le nom du node ou le contenu évoluent.

Toujours dans le système de clipping de la fenêtre ImGui : le texte est rendu via le ImDrawList courant, donc il reste correctement clipé dans le node.
​

Petites variantes possibles
Si tu veux que le texte soit collé exactement à la bordure droite du node, enlève le terme pin_radius * 2.0f dans x et garde juste un padding par rapport à rect_max.x.

Tu peux stocker le pin_radius et le padding dans ta config de thème Blueprint (comme UE/Unity) pour les ajuster facilement.

Si tu veux des labels colorés selon le type de pin, tu changes juste col avant AddText().

Si tu veux, tu peux me coller un bout de ton code actuel de rendu de pins, et je te propose une version plug‑and‑play adaptée à ton architecture Olympe (gestion des types de pins, shortcuts, etc.).