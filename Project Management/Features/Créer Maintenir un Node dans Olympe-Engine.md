Créer/Maintenir un Node dans Olympe-Engine
Voici la checklist complète pour bien intégrer un nouveau nœud :

1️⃣ DÉFINITION DU TYPE DE NODE
Étape 1.1 : Ajouter l'énumération
Fichier : Source/TaskSystem/TaskGraphTypes.h

C++
enum class TaskNodeType : uint8_t {
    // Existing types 0-5 (unchanged)
    AtomicTask = 0,
    Condition  = 1,
    Selector   = 2,
    Sequence   = 3,
    Decorator  = 4,
    Root       = 5,
    
    // Phase 1+ Visual Script extensions (6-17)
    EntryPoint  = 6,
    Branch      = 7,
    VSSequence  = 8,
    While       = 9,
    ForEach     = 10,
    DoOnce      = 11,
    Delay       = 12,
    GetBBValue  = 13,
    SetBBValue  = 14,
    MathOp      = 15,
    SubGraph    = 16,
    Switch      = 17,
    
    // ⭐ TON NOUVEAU NODE ICI
    MyCustomNode = 18  // ← Nouveau
};
📋 Checklist :

 Valeur entière unique > 17
 Nommé en PascalCase
 Pas d'espace dans le nom
Étape 1.2 : Ajouter la conversion string ↔ enum
Fichier : Source/TaskSystem/TaskGraphLoader.cpp

C++
TaskNodeType TaskGraphLoader::StringToNodeType(
    const std::string& s,
    const std::string& graphType,
    bool& outTypeOk)
{
    outTypeOk = true;
    
    // ... autres types ...
    
    if (s == "MyCustomNode") {
        return TaskNodeType::MyCustomNode;
    }
    
    outTypeOk = false;
    return TaskNodeType::AtomicTask;
}
Et la conversion inverse (VisualScriptNodeRenderer.cpp) :

C++
const char* GetNodeTypeLabel(TaskNodeType type)
{
    switch (type)
    {
        // ... autres types ...
        case TaskNodeType::MyCustomNode:
            return "MyCustomNode";
        default:
            return "Unknown";
    }
}
📋 Checklist :

 Bidirectionnel (string → enum et enum → string)
 Testé avec un roundtrip JSON
2️⃣ STRUCTURE DE DONNÉES
Étape 2.1 : Ajouter les champs au TaskNodeDefinition
Fichier : Source/TaskSystem/TaskGraphTemplate.h

C++
struct TaskNodeDefinition {
    int32_t NodeID = NODE_INDEX_NONE;
    std::string NodeName;
    TaskNodeType Type;
    
    // Execution pins
    std::vector<ExecPinConnection> ExecConnections;
    
    // Data pins (Phase 24)
    std::vector<DataPinDefinition> DataPins;
    std::vector<DataPinConnection> DataConnections;
    
    // Parameters (key-value pairs)
    std::unordered_map<std::string, ParameterBinding> Parameters;
    
    // ⭐ TON NOUVEAU CHAMP ICI
    struct MyCustomNodeData {
        std::string customProperty;
        float customValue = 0.0f;
        std::vector<std::string> customList;
    } myCustomData;  // ← Nouveau
};
📋 Checklist :

 Initialise les valeurs par défaut
 Type approprié (float, int, string, vector, etc.)
 Nommé my[Type]Data pour clarté
Étape 2.2 : Sérialisation JSON (Parse)
Fichier : Source/TaskSystem/TaskGraphLoader.cpp → ParseNodeV4()

C++
TaskNodeDefinition TaskGraphLoader::ParseNodeV4(
    const json& nodeJson,
    const std::string& graphType,
    std::vector<std::string>& outErrors)
{
    TaskNodeDefinition nd;
    nd.NodeID = JsonHelper::GetInt(nodeJson, "id", -1);
    nd.Type = StringToNodeType(JsonHelper::GetString(nodeJson, "type", ""), graphType, ...);
    
    // ⭐ TON PARSING ICI
    if (nd.Type == TaskNodeType::MyCustomNode) {
        nd.myCustomData.customProperty = JsonHelper::GetString(
            nodeJson, "customProp", "default");
        nd.myCustomData.customValue = JsonHelper::GetFloat(
            nodeJson, "customVal", 0.0f);
        
        // Charger la liste
        if (nodeJson.contains("customList") && nodeJson["customList"].is_array()) {
            for (const auto& item : nodeJson["customList"]) {
                nd.myCustomData.customList.push_back(item.get<std::string>());
            }
        }
    }
    
    return nd;
}
📋 Checklist :

 Gérer les champs manquants (valeurs par défaut)
 Utiliser JsonHelper::GetType() pour sécurité
 Validation des types (logguer les erreurs)
Étape 2.3 : Sérialisation JSON (Écriture)
Fichier : Source/BlueprintEditor/VisualScriptEditorPanel_FileOperations.cpp

C++
void VisualScriptEditorPanel::SerializeAndWrite()
{
    // ... code existant ...
    
    for (const auto& def : m_template.Nodes)
    {
        json n;
        n["id"] = def.NodeID;
        n["label"] = def.NodeName;
        n["type"] = GetNodeTypeLabel(def.Type);
        
        // ⭐ TON SERIALIZATION ICI
        if (def.Type == TaskNodeType::MyCustomNode) {
            n["customProp"] = def.myCustomData.customProperty;
            n["customVal"] = def.myCustomData.customValue;
            
            json listArray = json::array();
            for (const auto& item : def.myCustomData.customList) {
                listArray.push_back(item);
            }
            n["customList"] = listArray;
        }
        
        nodesArray.push_back(n);
    }
}
📋 Checklist :

 Tous les champs sérialisés
 Roundtrip JSON → objet → JSON sans perte
 Testé avec SaveAs()
3️⃣ CRÉATION DE NODE
Étape 3.1 : Initialisation lors de la création
Fichier : Source/BlueprintEditor/VisualScriptEditorPanel_NodeManagement.cpp

C++
int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    // ... code existant ...
    
    // ⭐ TON INITIALISATION ICI
    if (type == TaskNodeType::MyCustomNode) {
        // Initialiser les DataPins si besoin
        DataPinDefinition inPin;
        inPin.PinName = "Input";
        inPin.Dir = DataPinDir::Input;
        inPin.PinType = VariableType::String;
        def.DataPins.push_back(inPin);
        
        DataPinDefinition outPin;
        outPin.PinName = "Output";
        outPin.Dir = DataPinDir::Output;
        outPin.PinType = VariableType::String;
        def.DataPins.push_back(outPin);
        
        // Initialiser les custom data
        def.myCustomData.customProperty = "default";
        def.myCustomData.customValue = 1.0f;
    }
    
    // Ajouter à l'éditeur
    m_editorNodes.push_back(eNode);
    
    // Ajouter à la template + undo/redo
    m_undoStack.PushCommand(
        std::unique_ptr<ICommand>(new AddNodeCommand(def)),
        m_template);
    
    return newID;
}
📋 Checklist :

 Initialiser les DataPins (s'il y en a)
 Initialiser les données spécifiques
 Créer les pins dans le bon ordre (ils apparaîtront en bas)
 Pushcommand dans l'UndoStack
4️⃣ ÉDITION & PROPERTIES PANEL
Étape 4.1 : Ajouter un renderer de propriétés
Fichier : Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp

C++
void VisualScriptEditorPanel::RenderMyCustomNodeProperties(
    VSEditorNode& eNode,
    TaskNodeDefinition& def)
{
    // ── Blue header ─────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::Selectable(def.NodeName.c_str(), true, ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
    
    // ── Custom Property Editor ──────────────────────────
    ImGui::Text("Custom Property:");
    char buf[256];
    strcpy_s(buf, sizeof(buf), def.myCustomData.customProperty.c_str());
    if (ImGui::InputText("##customProp", buf, sizeof(buf))) {
        def.myCustomData.customProperty = buf;
        
        // Sync with template
        for (auto& node : m_template.Nodes) {
            if (node.NodeID == def.NodeID) {
                node.myCustomData.customProperty = buf;
                break;
            }
        }
        m_dirty = true;
    }
    
    ImGui::Spacing();
    ImGui::Text("Custom Value:");
    if (ImGui::InputFloat("##customVal", &def.myCustomData.customValue)) {
        for (auto& node : m_template.Nodes) {
            if (node.NodeID == def.NodeID) {
                node.myCustomData.customValue = def.myCustomData.customValue;
                break;
            }
        }
        m_dirty = true;
    }
    
    // ── List editor ─────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("Custom List:");
    ImGui::BeginChild("CustomListChild", ImVec2(0, 100), true);
    for (size_t i = 0; i < def.myCustomData.customList.size(); ++i) {
        ImGui::BulletText("%s", def.myCustomData.customList[i].c_str());
    }
    ImGui::EndChild();
    
    if (ImGui::Button("+##addItem")) {
        def.myCustomData.customList.push_back("new_item");
        m_dirty = true;
    }
}
📋 Checklist :

 Respecte le style (blue header, spacing)
 Tous les champs du node éditable
 Changes synchronisés avec m_template
 Marquer m_dirty = true après chaque modification
Étape 4.2 : Ajouter au dispatcher principal
Fichier : Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp → RenderProperties()

C++
void VisualScriptEditorPanel::RenderProperties()
{
    if (m_selectedNodeID == NODE_INDEX_NONE) {
        ImGui::Text("No node selected");
        return;
    }
    
    // Chercher le node
    VSEditorNode* eNode = nullptr;
    for (auto& en : m_editorNodes) {
        if (en.nodeID == m_selectedNodeID) {
            eNode = &en;
            break;
        }
    }
    
    if (!eNode) return;
    
    TaskNodeDefinition& def = eNode->def;
    
    // ⭐ TON DISPATCHER ICI
    if (def.Type == TaskNodeType::MyCustomNode) {
        RenderMyCustomNodeProperties(*eNode, def);
        return;  // Important : return après pour ne pas afficher le default
    }
    
    // ... autres types ...
    
    // Default
    ImGui::Text("Node: %s (%s)", def.NodeName.c_str(), GetNodeTypeLabel(def.Type));
}
📋 Checklist :

 Ajouter le case dans le switch/if
 Appeler ton renderer
 Vérifier que le return empêche les doubles renderings
5️⃣ RENDU SUR LE CANVAS
Étape 5.1 : Ajouter le style du node
Fichier : Source/BlueprintEditor/VisualScriptNodeRenderer.cpp

C++
VSNodeStyle GetNodeStyle(TaskNodeType type)
{
    switch (type) {
        case TaskNodeType::EntryPoint:  return VSNodeStyle::EntryPoint;
        case TaskNodeType::Branch:      return VSNodeStyle::FlowControl;
        case TaskNodeType::AtomicTask:  return VSNodeStyle::Action;
        case TaskNodeType::GetBBValue:  return VSNodeStyle::Data;
        case TaskNodeType::MathOp:      return VSNodeStyle::Data;
        
        // ⭐ TON STYLE ICI
        case TaskNodeType::MyCustomNode: return VSNodeStyle::Action;  // ou Data/FlowControl
        
        default: return VSNodeStyle::Action;
    }
}
Couleurs disponibles :

EntryPoint → Vert
FlowControl → Bleu
Action → Orange
Data → Violet
SubGraph → Teal
Delay → Jaune
📋 Checklist :

 Style choisi selon la fonction du node
 Couleur cohérente avec la catégorie
Étape 5.2 : Rendu des pins
Fichier : Source/BlueprintEditor/VisualScriptEditorPanel_PinHelpers.cpp

C++
// Exec input pins
std::vector<std::string> VisualScriptEditorPanel::GetExecInputPins(TaskNodeType type)
{
    switch (type) {
        case TaskNodeType::EntryPoint:
            return {};  // Pas d'entrée
        case TaskNodeType::GetBBValue:
        case TaskNodeType::MathOp:
            return {};  // Data-pure nodes
        
        // ⭐ TON NODE ICI
        case TaskNodeType::MyCustomNode:
            return {"In"};  // 1 exec entrée
        
        default:
            return {"In"};
    }
}

// Exec output pins
std::vector<std::string> VisualScriptEditorPanel::GetExecOutputPins(TaskNodeType type)
{
    switch (type) {
        case TaskNodeType::Branch:
            return {"True", "False"};
        case TaskNodeType::VSSequence:
            return {"Out0", "Out1", "Out2"};  // dynamic
        
        // ⭐ TON NODE ICI
        case TaskNodeType::MyCustomNode:
            return {"Out", "Error"};  // 2 sorties
        
        default:
            return {"Out"};
    }
}

// Data input pins
std::vector<std::pair<std::string, VariableType>> 
VisualScriptEditorPanel::GetDataInputPins(const TaskNodeDefinition& def)
{
    std::vector<std::pair<std::string, VariableType>> pins;
    
    // ⭐ TON NODE ICI
    if (def.Type == TaskNodeType::MyCustomNode) {
        // Utiliser DataPins du node (vient de AddNode())
        for (const auto& pin : def.DataPins) {
            if (pin.Dir == DataPinDir::Input) {
                pins.push_back({pin.PinName, pin.PinType});
            }
        }
    }
    
    return pins;
}

// Data output pins
std::vector<std::pair<std::string, VariableType>>
VisualScriptEditorPanel::GetDataOutputPins(const TaskNodeDefinition& def)
{
    std::vector<std::pair<std::string, VariableType>> pins;
    
    // ⭐ TON NODE ICI
    if (def.Type == TaskNodeType::MyCustomNode) {
        for (const auto& pin : def.DataPins) {
            if (pin.Dir == DataPinDir::Output) {
                pins.push_back({pin.PinName, pin.PinType});
            }
        }
    }
    
    return pins;
}
📋 Checklist :

 Exec pins en entrée définis
 Exec pins en sortie définis
 DataPins associés et typés
 Testé : pins visibles sur le canvas
6️⃣ VALIDATION
Étape 6.1 : Ajouter des validations
Fichier : Source/BlueprintEditor/VSGraphVerifier.cpp

C++
std::vector<ValidationIssue> VSGraphVerifier::Verify(
    const TaskGraphTemplate& tmpl)
{
    std::vector<ValidationIssue> issues;
    
    // ... autres validations ...
    
    // ⭐ TA VALIDATION ICI
    for (const auto& node : tmpl.Nodes) {
        if (node.Type == TaskNodeType::MyCustomNode) {
            // Valider que customProperty n'est pas vide
            if (node.myCustomData.customProperty.empty()) {
                ValidationIssue issue;
                issue.NodeID = node.NodeID;
                issue.NodeName = node.NodeName;
                issue.Severity = ValidationSeverity::Error;
                issue.Code = "E040_MyCustomNodeEmptyProperty";
                issue.Message = "MyCustomNode must have a custom property set";
                issues.push_back(issue);
            }
            
            // Valider que customValue est dans une plage valide
            if (node.myCustomData.customValue < 0.0f || node.myCustomData.customValue > 100.0f) {
                ValidationIssue issue;
                issue.NodeID = node.NodeID;
                issue.NodeName = node.NodeName;
                issue.Severity = ValidationSeverity::Warning;
                issue.Code = "W040_MyCustomNodeOutOfRange";
                issue.Message = "MyCustomNode customValue should be between 0 and 100";
                issues.push_back(issue);
            }
        }
    }
    
    return issues;
}
📋 Checklist :

 Validations logiques (null checks, ranges, etc.)
 Codes d'erreur uniques (E040, W040, etc.)
 Messages clairs en anglais
7️⃣ EXÉCUTION RUNTIME
Étape 7.1 : Handler d'exécution
Fichier : Source/TaskSystem/VSGraphExecutor.cpp

C++
void VSGraphExecutor::ExecuteFrame(EntityID entity,
                                   TaskRunnerComponent& runner,
                                   const TaskGraphTemplate& tmpl,
                                   LocalBlackboard& localBB,
                                   World* worldPtr,
                                   float dt)
{
    // ... code existant ...
    
    switch (node->Type) {
        // ... autres types ...
        
        // ⭐ TON HANDLER ICI
        case TaskNodeType::MyCustomNode:
            nextID = HandleMyCustomNode(currentID, runner, tmpl, localBB);
            break;
    }
}

// ⭐ TON HANDLER IMPLEMENTATION
int32_t VSGraphExecutor::HandleMyCustomNode(
    int32_t nodeID,
    TaskRunnerComponent& runner,
    const TaskGraphTemplate& tmpl,
    LocalBlackboard& localBB)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);
    if (!node) return NODE_INDEX_NONE;
    
    // Logique d'exécution
    SYSTEM_LOG << "[VSGraphExecutor] Executing MyCustomNode: "
               << node->myCustomData.customProperty << "\n";
    
    // Résoudre les data pins d'entrée
    TaskValue inputValue = ResolveDataPin(nodeID, "Input", runner, tmpl, localBB);
    
    // Exécuter la logique
    TaskValue outputValue = inputValue;  // ou ton traitement
    
    // Stocker le résultat dans le cache
    runner.DataPinCache["N" + std::to_string(nodeID) + ":Output"] = outputValue;
    
    // Retourner le prochain node
    const ExecPinConnection* connection = tmpl.FindExecConnection(nodeID, "Out");
    if (connection) {
        return connection->TargetNodeID;
    }
    
    return NODE_INDEX_NONE;
}
En-tête :
Fichier : Source/TaskSystem/VSGraphExecutor.h

C++
class VSGraphExecutor {
private:
    int32_t HandleMyCustomNode(
        int32_t nodeID,
        TaskRunnerComponent& runner,
        const TaskGraphTemplate& tmpl,
        LocalBlackboard& localBB);
};
📋 Checklist :

 Handler implémenté dans VSGraphExecutor
 Case ajouté au switch
 DataPins résolus d'entrée
 Résultat stocké en sortie
 Prochain node retourné
Étape 7.2 : Test d'exécution
Fichier : Tests/BlueprintEditor/MyCustomNodeTest.cpp (nouveau)

C++
#include <cassert>
#include "../Source/TaskSystem/VSGraphExecutor.h"
#include "../Source/TaskSystem/TaskGraphTemplate.h"

void Test1_MyCustomNodeCreation() {
    TaskGraphTemplate tmpl;
    
    TaskNodeDefinition node;
    node.NodeID = 1;
    node.Type = TaskNodeType::MyCustomNode;
    node.myCustomData.customProperty = "test";
    node.myCustomData.customValue = 42.0f;
    
    tmpl.Nodes.push_back(node);
    
    assert(tmpl.Nodes.size() == 1);
    assert(tmpl.Nodes[0].myCustomData.customValue == 42.0f);
}

void Test2_MyCustomNodeExecution() {
    // ... setup tmpl, runner, etc. ...
    
    VSGraphExecutor executor;
    executor.ExecuteFrame(1, runner, tmpl, localBB, nullptr, 0.016f);
    
    // Vérifier que le resultat est correct
    // assert(runner.DataPinCache["N1:Output"] == ...);
}
📋 Checklist :

 Tests unitaires pour la création
 Tests pour l'exécution
 Tests pour la sérialisation/désérialisation
8️⃣ SIMULATION & GRAPHS SIMULATOR
Étape 8.1 : Support du simulateur
Fichier : Source/TaskSystem/GraphSimulator.cpp (s'il existe)

C++
void GraphSimulator::StepSimulation() {
    // ... code existant ...
    
    // ⭐ TON NODE ICI
    if (currentNode->Type == TaskNodeType::MyCustomNode) {
        SimulationResult result = SimulateMyCustomNode(*currentNode, simulationState);
        nextNodeID = result.nextNodeID;
        // Enregistrer les résultats pour le debugger
        m_stepTrace.push_back({currentNode->NodeID, "MyCustomNode", result.description});
    }
}
📋 Checklist :

 Peut être simulé frame-by-frame
 Traçage des étapes
 Pas d'effets secondaires non prévisibles
9️⃣ TRAÇAGE & LOGS
Étape 9.1 : Logging et message tracing
Fichier : Partout dans ton code

C++
// Création
SYSTEM_LOG << "[VSEditor] Created MyCustomNode #" << nodeID 
           << " with property '" << def.myCustomData.customProperty << "'\n";

// Édition
SYSTEM_LOG << "[VSEditor] Modified MyCustomNode #" << nodeID 
           << " property to '" << newValue << "'\n";

// Sérialisation
SYSTEM_LOG << "[TaskGraphLoader] Parsed MyCustomNode #" << nd.NodeID 
           << " from JSON\n";

// Exécution
SYSTEM_LOG << "[VSGraphExecutor] Executing MyCustomNode #" << nodeID 
           << " (custom value: " << node->myCustomData.customValue << ")\n";

// Erreur
SYSTEM_LOG << "[VSGraphExecutor] WARNING: MyCustomNode #" << nodeID 
           << " has invalid property\n";
Format :

[Component] : Component name en majuscules
Message : Description claire en anglais
 #ID : Node/object ID pour traçabilité
📋 Checklist :

 Logs aux points clés (création, modification, exécution, erreurs)
 Format cohérent [Component] message
 Niveau de détail approprié
🔟 TESTS & VALIDATION
Étape 10.1 : Checklist de test complète
Test	Fichier	Status
JSON roundtrip (save/load)	Phase*Test.cpp	[ ]
Création node	Phase*Test.cpp	[ ]
Édition propriétés	Phase*Test.cpp	[ ]
Execution simple	Phase*Test.cpp	[ ]
Undo/Redo	Phase*Test.cpp	[ ]
Validation	Phase*Test.cpp	[ ]
DataPins connexion	Phase*Test.cpp	[ ]
SubGraph recursion	Phase*Test.cpp	[ ]
C++
void Test_MyCustomNodeRoundTrip() {
    // 1. Créer
    int nodeID = panel.AddNode(TaskNodeType::MyCustomNode, 100, 200);
    
    // 2. Sauvegarder
    panel.Save("test_mycustomnode.ats");
    
    // 3. Charger
    TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromFile("test_mycustomnode.ats", errors);
    
    // 4. Vérifier
    assert(tmpl->Nodes.size() == 1);
    assert(tmpl->Nodes[0].Type == TaskNodeType::MyCustomNode);
    assert(tmpl->Nodes[0].myCustomData.customProperty == original);
}
📋 RÉSUMÉ : CHECKLIST FINALE
Code
┌─ DÉFINITION (Fichiers modifiés: 2)
│  ├─ TaskGraphTypes.h — enum value
│  └─ TaskGraphLoader.cpp — StringToNodeType()
│
├─ STRUCTURE (Fichiers modifiés: 1)
│  └─ TaskGraphTemplate.h — TaskNodeDefinition champs
│
├─ SÉRIALISATION (Fichiers modifiés: 2)
│  ├─ TaskGraphLoader.cpp — ParseNodeV4()
│  └─ VisualScriptEditorPanel_FileOperations.cpp — SerializeAndWrite()
│
├─ CRÉATION (Fichiers modifiés: 1)
│  └─ VisualScriptEditorPanel_NodeManagement.cpp — AddNode()
│
├─ ÉDITION (Fichiers modifiés: 2)
│  └─ VisualScriptEditorPanel_Properties.cpp — RenderMyCustomNodeProperties()
│  └─ VisualScriptEditorPanel_Properties.cpp — RenderProperties() dispatcher
│
├─ RENDU (Fichiers modifiés: 3)
│  ├─ VisualScriptNodeRenderer.cpp — GetNodeStyle()
│  └─ VisualScriptEditorPanel_PinHelpers.cpp — GetExecInputPins(), GetExecOutputPins(), etc.
│
├─ VALIDATION (Fichiers modifiés: 1)
│  └─ VSGraphVerifier.cpp — Verify()
│
├─ EXÉCUTION (Fichiers modifiés: 2)
│  ├─ VSGraphExecutor.cpp — HandleMyCustomNode() + case switch
│  └─ VSGraphExecutor.h — déclaration
│
├─ TESTS (Fichiers créés: 1)
│  └─ Tests/BlueprintEditor/MyCustomNodeTest.cpp — tests unitaires
│
└─ LOGS (Fichiers modifiés: 10+)
   └─ Ajouter SYSTEM_LOG partout