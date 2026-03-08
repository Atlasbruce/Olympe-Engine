/**
 * @file NodeTypeCatalogue.h
 * @brief Référentiel des types de nodes ATS — Phase 1.2
 * @author Olympe Engine / Atlasbruce
 * @date 2026-03-08
 *
 * @details
 * Singleton de chargement et d'accès aux descripteurs de types de nodes ATS.
 * Les types sont définis dans des fichiers JSON v2 dans Blueprints/Catalogues/.
 */

#pragma once

#include "../json_helper.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// PinType — types de pins ATS
// ============================================================================

/**
 * @enum PinType
 * @brief Types de pins supportés dans les catalogues ATS
 */
enum class PinType : uint8_t {
    Exec = 0,   ///< Lien d'exécution (flux de contrôle)
    Int,        ///< Entier 32 bits
    Float,      ///< Flottant 32 bits
    Bool,       ///< Booléen
    String,     ///< Chaîne de caractères
    Vector,     ///< Vecteur 2D — utilise le constructeur Vector(x, y)
    EntityRef,  ///< Référence à une entité du monde
    GraphRef,   ///< Référence à un graphe externe (filepath)
    AnimRef,    ///< Référence à une animation (id ou filepath)
    SpriteRef,  ///< Référence à un sprite (id ou filepath)
    Any         ///< Type générique (pour les nodes utilitaires)
};

// ============================================================================
// Descripteurs
// ============================================================================

/**
 * @struct PinDescriptor
 * @brief Description d'un pin d'un type de node
 */
struct PinDescriptor {
    std::string id;
    std::string label;
    PinType     type     = PinType::Any;
    bool        required = false;
};

/**
 * @struct ParameterDescriptor
 * @brief Description d'un paramètre éditeur d'un type de node
 */
struct ParameterDescriptor {
    std::string name;
    PinType     type;
    bool        required     = false;
    std::string defaultValue; ///< Stocké en string, parsé selon le type au besoin
    std::string description;
};

/**
 * @struct NodeTypeDescriptor
 * @brief Descripteur complet d'un type de node ATS
 */
struct NodeTypeDescriptor {
    std::string id;
    std::string name;
    std::string category;
    std::string catalogType; ///< "Actions", "Conditions", "Decorators", "Composites", "Services"
    std::string description;
    std::string tooltip;
    std::string color;       ///< Couleur hexadécimale "#RRGGBB"

    std::vector<PinDescriptor>       inputPins;
    std::vector<PinDescriptor>       outputPins;
    std::vector<ParameterDescriptor> parameters;
};

// ============================================================================
// NodeTypeCatalogue
// ============================================================================

/**
 * @class NodeTypeCatalogue
 * @brief Singleton de gestion des catalogues de types de nodes ATS
 *
 * @details
 * Charge les fichiers JSON v2 depuis un dossier et expose des méthodes
 * d'accès par id, par catalogType ou par catégorie.
 *
 * Usage :
 * @code
 *   NodeTypeCatalogue::Get().LoadFromDirectory("Blueprints/Catalogues");
 *   const NodeTypeDescriptor* desc = NodeTypeCatalogue::Get().GetType("GotoPosition");
 * @endcode
 */
class NodeTypeCatalogue {
public:
    /**
     * @brief Accès au singleton
     */
    static NodeTypeCatalogue& Get();

    /**
     * @brief Charge tous les fichiers JSON (sauf *.backup) depuis un dossier
     * @param cataloguePath Chemin du dossier contenant les fichiers catalogue
     * @return true si au moins un catalogue a été chargé
     */
    bool LoadFromDirectory(const std::string& cataloguePath);

    /**
     * @brief Charge un fichier JSON de catalogue
     * @param filePath Chemin absolu ou relatif du fichier
     * @return true si chargé avec succès
     */
    bool LoadCatalogueFile(const std::string& filePath);

    /**
     * @brief Récupère un descripteur de type par son identifiant
     * @param id Identifiant unique du type (ex: "GotoPosition")
     * @return Pointeur const ou nullptr si non trouvé
     */
    const NodeTypeDescriptor* GetType(const std::string& id) const;

    /**
     * @brief Récupère tous les types d'un catalogType donné
     * @param catalogType "Actions", "Conditions", "Decorators", "Composites" ou "Services"
     * @return Vecteur de pointeurs const
     */
    std::vector<const NodeTypeDescriptor*> GetByCatalogType(const std::string& catalogType) const;

    /**
     * @brief Récupère tous les types d'une catégorie donnée
     * @param category Ex: "Movement", "Logic", "System"
     * @return Vecteur de pointeurs const
     */
    std::vector<const NodeTypeDescriptor*> GetByCategory(const std::string& category) const;

    /**
     * @brief Retourne tous les types enregistrés
     */
    const std::vector<NodeTypeDescriptor>& GetAll() const;

    /**
     * @brief Vérifie si un type est enregistré
     * @param id Identifiant du type
     */
    bool HasType(const std::string& id) const;

    /**
     * @brief Vide les types chargés et recharge depuis le dossier configuré
     */
    void Reload();

    /**
     * @brief Nombre total de types chargés
     */
    size_t Count() const;

    /**
     * @brief Convertit un PinType en string
     */
    static std::string PinTypeToString(PinType t);

    /**
     * @brief Convertit un string en PinType (retourne Any si inconnu)
     */
    static PinType StringToPinType(const std::string& s);

private:
    NodeTypeCatalogue() = default;
    ~NodeTypeCatalogue() = default;

    NodeTypeCatalogue(const NodeTypeCatalogue&) = delete;
    NodeTypeCatalogue& operator=(const NodeTypeCatalogue&) = delete;

    /**
     * @brief Parse un objet JSON de catalogue v2 et insère les types
     * @param j Objet JSON racine du catalogue
     * @param catalogType Type de catalogue (ex: "Actions")
     */
    void ParseCatalogueJson(const json& j, const std::string& catalogType);

    std::vector<NodeTypeDescriptor> m_types;
    std::string m_cataloguePath;
};

} // namespace NodeGraph
} // namespace Olympe
