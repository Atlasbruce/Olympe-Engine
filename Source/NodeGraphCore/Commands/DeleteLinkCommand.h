/**
 * @file DeleteLinkCommand.h
 * @brief Command to delete a link
 * @author Olympe Engine - NodeGraph Core v2.0
 * @date 2025-02-19
 */

#pragma once

#include "../CommandSystem.h"
#include "../GraphDocument.h"

namespace Olympe
{
namespace NodeGraph
{
using namespace NodeGraphTypes;
{
namespace Commands
{

/**
 * @class DeleteLinkCommand
 * @brief Commande pour supprimer un lien
 */
class DeleteLinkCommand : public ICommand
{
public:
    /**
     * @brief Constructeur
     * @param document GraphDocument cible
     * @param linkId ID du lien à supprimer
     */
    DeleteLinkCommand(GraphDocument* document, uint32_t linkId);
    
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override;
    
private:
    GraphDocument* m_document = nullptr;
    uint32_t m_linkId = 0;
    LinkData m_deletedLink;
    bool m_wasExecuted = false;
};

} // namespace Commands
} // namespace NodeGraph
} // namespace Olympe
