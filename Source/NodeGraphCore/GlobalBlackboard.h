/**
 * @file GlobalBlackboard.h
 * @brief Singleton des variables globales partagées entre tous les graphes ATS — Phase 1.3
 * @author Olympe Engine / Atlasbruce
 * @date 2026-03-08
 *
 * @details
 * Le GlobalBlackboard expose les variables partagées entre TOUS les graphes ATS.
 * Il encapsule un BlackboardSystem interne et ajoute la persistance JSON
 * (load/save du fichier Blueprints/global_blackboard.json).
 *
 * Format JSON global_blackboard.json :
 * @code
 * {
 *   "schema_version": 2,
 *   "type": "GlobalBlackboard",
 *   "metadata": { ... },
 *   "variables": [
 *     { "name": "player_health", "type": "Float",  "value": 100.0 },
 *     { "name": "player_pos",    "type": "Vector", "x": 0.0, "y": 0.0 },
 *     { "name": "wave_count",    "type": "Int",    "value": 0 }
 *   ]
 * }
 * @endcode
 */

#pragma once

#include "BlackboardSystem.h"
#include "../json_helper.h"
#include <string>

namespace Olympe {
namespace NodeGraph {

/**
 * @class GlobalBlackboard
 * @brief Singleton des variables blackboard partagées entre tous les graphes ATS
 *
 * @details
 * Wraps un BlackboardSystem et ajoute le chargement/sauvegarde JSON.
 * Le flag dirty indique si une sauvegarde est nécessaire.
 *
 * Usage :
 * @code
 *   GlobalBlackboard::Get().LoadFromFile("Blueprints/global_blackboard.json");
 *   GlobalBlackboard::Get().SetVar("player_health", val);
 * @endcode
 */
class GlobalBlackboard {
public:
    /**
     * @brief Accès au singleton
     */
    static GlobalBlackboard& Get();

    // ========================================================================
    // Persistance JSON
    // ========================================================================

    /**
     * @brief Charge les variables depuis un fichier JSON
     * @param filepath Chemin du fichier (ex: "Blueprints/global_blackboard.json")
     * @return true si chargé avec succès
     */
    bool LoadFromFile(const std::string& filepath);

    /**
     * @brief Sauvegarde les variables vers un fichier JSON
     * @param filepath Chemin de destination
     * @return true si sauvegardé avec succès
     */
    bool SaveToFile(const std::string& filepath) const;

    // ========================================================================
    // Accès au BlackboardSystem sous-jacent
    // ========================================================================

    /**
     * @brief Accès direct au BlackboardSystem (non-const)
     */
    BlackboardSystem& GetBlackboard();

    /**
     * @brief Accès direct au BlackboardSystem (const)
     */
    const BlackboardSystem& GetBlackboard() const;

    // ========================================================================
    // Raccourcis (délèguent à m_blackboard)
    // ========================================================================

    /** @brief Vérifie si une variable existe */
    bool HasVar(const std::string& name) const;

    /** @brief Récupère un pointeur const sur la valeur, ou nullptr */
    const BlackboardValue* GetVar(const std::string& name) const;

    /**
     * @brief Met à jour la valeur d'une variable existante
     * @return true si mise à jour, false si non trouvée ou type incompatible
     */
    bool SetVar(const std::string& name, const BlackboardValue& value);

    /**
     * @brief Crée une nouvelle variable
     * @return true si créée, false si nom dupliqué
     */
    bool CreateVar(const std::string& name, BlackboardType type,
                   const BlackboardValue& initialValue);

    /**
     * @brief Supprime une variable
     * @return true si supprimée
     */
    bool RemoveVar(const std::string& name);

    /**
     * @brief Renomme une variable
     * @return true si renommée
     */
    bool RenameVar(const std::string& oldName, const std::string& newName);

    // ========================================================================
    // Métadonnées
    // ========================================================================

    /** @brief Retourne le chemin du fichier courant */
    std::string GetFilePath() const;

    /** @brief Retourne true si des modifications non sauvegardées existent */
    bool IsDirty() const;

    /** @brief Marque le blackboard comme modifié */
    void MarkDirty();

    /** @brief Efface le flag dirty */
    void ClearDirty();

    // ========================================================================
    // Sérialisation JSON (pour l'intégration éditeur ATS)
    // ========================================================================

    /**
     * @brief Sérialise le blackboard global au format JSON v2
     * @return Objet JSON avec schema_version, type, metadata, variables
     */
    json ToJson() const;

    /**
     * @brief Désérialise depuis un objet JSON v2
     * @param j Objet JSON produit par ToJson() ou un fichier global_blackboard.json
     * @return true si désérialisé avec succès
     */
    bool FromJson(const json& j);

private:
    GlobalBlackboard() = default;
    ~GlobalBlackboard() = default;

    GlobalBlackboard(const GlobalBlackboard&) = delete;
    GlobalBlackboard& operator=(const GlobalBlackboard&) = delete;

    BlackboardSystem m_blackboard;
    std::string m_filePath;
    bool m_isDirty = false;
};

} // namespace NodeGraph
} // namespace Olympe
