# 🎯 DIRECTIVES AGENT COPILOT - C++14 STRICT

## 📌 INSTRUCTIONS GÉNÉRALES

- **Standard**: C++14 strict, aucune fonctionnalité C++17/20
- **Plateforme**: Compatible Windows (MSVC) + Linux (GCC/Clang)
- **Warnings**: 0 toléré, compilation avec `-Wall -Wextra`
- **Namespace**: TOUT le code dans `namespace Olympe { }`
- **Validation**: Compilation mentale obligatoire avant soumission

---

## 🚫 INTERDICTIONS C++17/20

- ❌ `std::optional`, `std::variant`, `std::any`
- ❌ Structured bindings: `auto& [key, value]`
- ❌ `if constexpr`
- ❌ `std::string_view`
- ❌ Fold expressions: `(... + args)`
- ❌ Inline variables
- ❌ `[[nodiscard]]`, `[[maybe_unused]]` (C++17 attributes)
- ❌ Init-statements in `if/switch`: `if (auto x = foo(); x > 0)`
- ❌ Class template argument deduction (CTAD)

---

## ✅ SYNTAXE C++14 AUTORISÉE

- ✅ `auto` pour inférence de type
- ✅ Lambda expressions avec captures: `[&]`, `[=]`, `[this]`
- ✅ Range-based for loops: `for (const auto& item : container)`
- ✅ `constexpr` fonctions/variables
- ✅ `nullptr`
- ✅ `enum class`
- ✅ `override`, `final`
- ✅ Uniform initialization: `MyStruct{1, 2, 3}`
- ✅ Move semantics: `std::move()`, `&&`

---

## 📦 GESTION JSON (nlohmann/json)

### **INTERDICTIONS**
- ❌ Accès direct: `json["key"].get<T>()`
- ❌ Vérification manuelle: `if (json.contains("key"))`
- ❌ Array avec initialisateurs: `json::array({"a", "b"})`
- ❌ Object avec initialisateurs: `json::object({{"k", "v"}})`

### **OBLIGATIONS**
- ✅ Utiliser `json_get_int(obj, "key", defaultValue)`
- ✅ Utiliser `json_get_float(obj, "key", defaultValue)`
- ✅ Utiliser `json_get_string(obj, "key", defaultValue)`
- ✅ Utiliser `json_get_bool(obj, "key", defaultValue)`
- ✅ Créer arrays vides: `json arr = json::array();`
- ✅ Ajouter items: `arr.push_back(item);`
- ✅ Créer objects vides: `json obj = json::object();`
- ✅ Ajouter pairs: `obj["key"] = value;`

### **ITÉRATION JSON C++14**
```cpp
// ✅ CORRECT
for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it)
{
    std::string key = it.key();
    const auto& value = it.value();
    // Traitement
}

// ❌ INTERDIT
for (auto& [key, value] : jsonObj.items())
{
    // C++17 structured binding
}
```

---

## 🗂️ STRUCTURES ET CLASSES

### **INITIALISATION MEMBRES**
- ✅ Toujours fournir valeurs par défaut dans la déclaration
- ✅ Ordre: types simples, puis complexes, puis pointeurs
```cpp
struct MyStruct {
    int id = 0;
    float ratio = 1.0f;
    bool active = false;
    std::string name;
    std::vector<int> data;
    MyClass* ptr = nullptr;
};
```

### **DÉCLARATION AVANT UTILISATION**
- ✅ Déclarer struct/class dans header AVANT toute utilisation
- ✅ Ordre dans `.h`: forward declarations → enums → structs → classes
- ✅ Déclarer variables membres AVANT de les utiliser dans `.cpp`

### **ENUM CLASS**
- ✅ Toujours utiliser `enum class` (pas `enum`)
- ✅ Spécifier type si nécessaire: `enum class Type : uint8_t`
```cpp
enum class NodeType : uint8_t {
    Selector,
    Sequence,
    Action
};
```

---

## 🔒 NAMESPACE

### **RÈGLES STRICTES**
- ✅ TOUT le code implémentation dans `namespace Olympe { }`
- ✅ Fermer namespace avec commentaire: `} // namespace Olympe`
- ❌ Aucun namespace anonyme orphelin
- ❌ Aucune fonction globale hors namespace
- ❌ Aucune variable globale hors namespace

### **STRUCTURE FICHIER .cpp**
```cpp
#include "Header.h"
// Autres includes

using json = nlohmann::json;  // ✅ OK avant namespace

namespace Olympe
{
    // ✅ Constants
    constexpr float MAX_VALUE = 10.0f;
    
    // ✅ Implémentations
    void MyClass::MyMethod() { }
    
} // namespace Olympe
```

---

## 🖥️ COMPATIBILITÉ PLATEFORME

### **FONCTIONS SYSTÈME**
- ❌ `localtime()` → warning Windows
- ✅ Utiliser `localtime_s()` (Windows) + `localtime_r()` (Linux)
```cpp
std::tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &timeValue);
#else
    localtime_r(&timeValue, &timeInfo);
#endif
```

### **CONVERSIONS**
- ✅ Cast explicite pour éviter warnings
```cpp
uint8_t byte = static_cast<uint8_t>(value);
float ratio = static_cast<float>(intNum) / intDen;
```

### **STRINGS SÉCURISÉES**
- ❌ `strcpy()`, `strcat()`, `sprintf()`
- ✅ `strcpy_s()`, `strcat_s()`, `snprintf()`
- ✅ Préférer `std::string` quand possible

---

## 🔗 API TIERCES (ImGui, ImNodes, SDL3)

### **VÉRIFICATION OBLIGATOIRE**
- ✅ Vérifier signature exacte dans la documentation
- ✅ Compter le nombre d'arguments requis
- ✅ Vérifier le type de retour
- ✅ Vérifier si pointeur ou référence

### **EXEMPLES CRITIQUES**
```cpp
// ImGui::GetWindowDrawList()
ImDrawList* drawList = ImGui::GetWindowDrawList();  // ✅ Retourne pointeur

// ImNodes::Link()
ImNodes::Link(linkId, startPinId, endPinId);  // ✅ 3 arguments

// json::array()
json arr = json::array();  // ✅ SANS arguments
```

### **POINTEURS IMGUI**
- ✅ Toujours vérifier `!= nullptr` avant utilisation
- ✅ Ne jamais stocker de pointeur ImGui entre frames
```cpp
ImDrawList* drawList = ImGui::GetWindowDrawList();
if (drawList != nullptr)  // ✅ Vérification
{
    drawList->AddLine(...);
}
```

---

## 📄 FICHIERS HEADER (.h)

### **STRUCTURE OBLIGATOIRE**
1. Pragma once ou guards
2. Includes (système → tiers → projet)
3. Forward declarations
4. Namespace opening
5. Enums
6. Structs/Classes
7. Namespace closing

### **DÉCLARATIONS MEMBRES**
- ✅ Ordre: `public` → `protected` → `private`
- ✅ Ordre dans section: méthodes → variables
- ✅ Initialiser variables membres dans déclaration
```cpp
class MyClass {
public:
    void Method();
    
private:
    void HelperMethod();
    
    int m_value = 0;        // ✅ Valeur par défaut
    std::string m_name;     // ✅ OK (std::string s'initialise)
    float* m_ptr = nullptr; // ✅ Valeur par défaut
};
```

### **FORWARD DECLARATIONS**
- ✅ Utiliser quand possible pour réduire dépendances
```cpp
// Forward declarations
struct SDL_Window;
struct ImGuiContext;
class MyClass;

// Utiliser pointeurs/références uniquement
SDL_Window* m_window;
```

---

## 📝 FICHIERS IMPLÉMENTATION (.cpp)

### **INCLUDES**
- ✅ Header correspondant en premier
- ✅ Headers système ensuite
- ✅ Headers tiers ensuite
- ✅ Headers projet à la fin
```cpp
#include "MyClass.h"        // ✅ En premier
#include <vector>           // ✅ Système
#include <SDL3/SDL.h>       // ✅ Tiers
#include "../Helper.h"      // ✅ Projet
```

### **USING DECLARATIONS**
- ✅ `using` au niveau fichier avant namespace
- ❌ `using namespace std;` interdit
```cpp
using json = nlohmann::json;  // ✅ OK

namespace Olympe {
    // Implémentations
}
```

### **ORDRE MÉTHODES**
- ✅ Suivre l'ordre du header
- ✅ Constructeur/Destructeur en premier
- ✅ Méthodes publiques avant privées
- ✅ Helpers statiques à la fin

---

## 🧪 VALIDATION PRÉ-COMMIT

### **CHECKLIST OBLIGATOIRE**
- [ ] Aucune fonctionnalité C++17/20 utilisée
- [ ] Tout le code dans `namespace Olympe`
- [ ] Tous les accès JSON utilisent `json_get_*()`
- [ ] `json::array()` et `json::object()` sans arguments
- [ ] Itération JSON avec `.begin()/.end()` et `it.key()/it.value()`
- [ ] `localtime_s()` avec `#ifdef _WIN32`
- [ ] Toutes les structs ont valeurs par défaut
- [ ] Toutes les variables membres déclarées dans header
- [ ] Toutes les fonctions déclarées sont implémentées
- [ ] Signature API tierces vérifiées
- [ ] Aucun pointeur non vérifié
- [ ] Casts explicites pour conversions
- [ ] Namespace fermé avec commentaire
- [ ] Includes dans le bon ordre
- [ ] Forward declarations utilisées quand possible

### **COMPILATION MENTALE**
- ✅ Relire TOUT le code ligne par ligne
- ✅ Vérifier chaque déclaration/définition
- ✅ Vérifier chaque appel de fonction
- ✅ Vérifier chaque accès mémoire
- ✅ Simuler la compilation dans sa tête
- ✅ Anticiper warnings potentiels

---

## 🎯 PATTERNS COURANTS

### **ITÉRATION CONTAINER**
```cpp
// ✅ Range-based for
for (const auto& item : container) { }

// ✅ Itérateur classique
for (auto it = container.begin(); it != container.end(); ++it) { }

// ✅ Index (si besoin)
for (size_t i = 0; i < container.size(); ++i) { }
```

### **INITIALISATION CONDITIONNELLE**
```cpp
// ❌ INTERDIT (C++17)
if (auto ptr = getPtr(); ptr != nullptr) { }

// ✅ C++14
auto ptr = getPtr();
if (ptr != nullptr) { }
```

### **MAPS/SETS**
```cpp
// ✅ Find + check
auto it = map.find(key);
if (it != map.end()) {
    const auto& value = it->second;
}

// ✅ Insert with check
auto result = map.insert({key, value});
if (result.second) {
    // Inserted
}
```

### **LAMBDA CAPTURES**
```cpp
// ✅ Capture by value
[=]() { return value; }

// ✅ Capture by reference
[&]() { modifyValue(); }

// ✅ Specific captures
[this, &var]() { }

// ✅ Mutable lambda
[=]() mutable { value++; }
```

---

## 🚨 ERREURS FRÉQUENTES À ÉVITER

### **1. Code orphelin hors namespace**
```cpp
// ❌ ERREUR
} // namespace Olympe

void MyFunction() { }  // Orphelin!
```

### **2. Structured binding C++17**
```cpp
// ❌ ERREUR
for (auto& [k, v] : map) { }

// ✅ CORRECT
for (auto it = map.begin(); it != map.end(); ++it) {
    auto& k = it->first;
    auto& v = it->second;
}
```

### **3. json::array avec arguments**
```cpp
// ❌ ERREUR
json arr = json::array({"a", "b"});

// ✅ CORRECT
json arr = json::array();
arr.push_back("a");
arr.push_back("b");
```

### **4. Variable non déclarée**
```cpp
// ❌ ERREUR dans .cpp
m_myVar = 5;  // m_myVar pas dans le header!

// ✅ CORRECT: Ajouter dans header
class MyClass {
    int m_myVar = 0;
};
```

### **5. Fonction déclarée non implémentée**
```cpp
// ❌ ERREUR: déclaré dans .h
void MyFunction();

// Mais jamais implémenté dans .cpp
// → Link error!
```

### **6. Accès JSON non protégé**
```cpp
// ❌ ERREUR
int val = json["key"].get<int>();

// ✅ CORRECT
int val = json_get_int(json, "key", 0);
```

### **7. localtime() non sécurisé**
```cpp
// ❌ ERREUR (warning Windows)
std::localtime(&time);

// ✅ CORRECT
#ifdef _WIN32
    localtime_s(&timeInfo, &time);
#else
    localtime_r(&time, &timeInfo);
#endif
```

### **8. Membres sans valeur par défaut**
```cpp
// ❌ ERREUR
struct Data {
    int value;  // Non initialisé!
};

// ✅ CORRECT
struct Data {
    int value = 0;
};
```

---

## 📊 TEMPLATE DE VALIDATION

```
✅ C++14 STRICT
- Aucun `std::optional`, `std::variant`, structured binding
- Itérateurs classiques avec .begin()/.end()
- Pas de `if constexpr`, `std::string_view`

✅ JSON HELPERS
- Tous les accès via json_get_*()
- json::array() sans arguments
- Itération avec it.key()/it.value()

✅ NAMESPACE
- 100% du code dans namespace Olympe
- Fermé avec commentaire
- Aucun code orphelin

✅ DÉCLARATIONS
- Toutes variables membres dans header
- Toutes fonctions implémentées
- Structs avec valeurs par défaut

✅ PLATEFORME
- localtime_s() avec #ifdef _WIN32
- Casts explicites
- Pointeurs vérifiés

✅ API TIERCES
- Signatures vérifiées
- Nombre d'arguments correct
- Types de retour respectés

✅ COMPILATION MENTALE
- Code relu ligne par ligne
- 0 erreurs prévisibles
- 0 warnings prévisibles
```

---

## 🔧 COMMANDE À RAPPELER

**"CRITICAL: C++14 strict. Use json_get_*() helpers. All code in namespace Olympe. Declare before use. Initialize all members. Use localtime_s with #ifdef. Verify API signatures. Mental compilation mandatory."**

---

## 📚 RÉFÉRENCES RAPIDES

### **Création JSON Array**
```cpp
json arr = json::array();
arr.push_back(item);
```

### **Itération JSON**
```cpp
for (auto it = obj.begin(); it != obj.end(); ++it) {
    std::string key = it.key();
    auto& val = it.value();
}
```

### **Temps portable**
```cpp
std::tm info;
#ifdef _WIN32
    localtime_s(&info, &t);
#else
    localtime_r(&t, &info);
#endif
```

### **Struct initialisée**
```cpp
struct S {
    int x = 0;
    float y = 1.0f;
    bool z = false;
};
```

### **Map lookup sécurisé**
```cpp
auto it = map.find(key);
if (it != map.end()) {
    value = it->second;
}
```

---

**🎯 RÈGLE D'OR**: Si tu doutes, vérifie dans la doc C++14, compile mentalement, et teste sur papier!

---

## 📝 HISTORIQUE DES RÉVISIONS

- **2025-02-16**: Création initiale après analyse des erreurs PR #267
- Objectif: Empêcher récurrence des erreurs de compilation C++17/namespace/JSON
