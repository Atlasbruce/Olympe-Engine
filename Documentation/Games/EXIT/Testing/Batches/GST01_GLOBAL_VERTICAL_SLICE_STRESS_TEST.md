# EXIT — GST01 : Global Vertical Slice Stress Test

> **Statut :** 🟢 REFERENCE — 25 septembre 2026  
> **Rôle :** test de non-régression conceptuel de la chaîne causale complète du Vertical Slice ; il ne prescrit aucune décision NPC.

## Chaîne éprouvée

```text
QUEST → WORLD → PARTICIPANTS → PARTIAL KNOWLEDGE → EXPLORATION
→ RESOURCE COMPETITION → COMMUNICATION → PSYCHO-SOCIAL APPRAISAL
→ STRATEGY → GOAP / PLAN → ACTION → WORLD CHANGES → REEVALUATION
→ EXIT → EVACUATION → END ATTEMPT → DEBRIEF
```

## État initial

Player P est près du Generator ; NPC A est en zone centrale ; NPC B est dans la salle Batteries, légèrement blessé et seul détenteur initial du Generator Code. Les inventaires sont vides. Aucun participant ne connaît topologie ni Quest Graph ; tous connaissent seulement le briefing Observer et leurs éventuelles Knowledge initiales explicites.

## Parcours de référence

| Moment | Fait World / information légitime | Propriété éprouvée |
|---|---|---|
| M1 | P découvre Generator ; A explore ; B prend une Battery sans connaître son usage. | découverte ≠ possession de la solution |
| M2 | P lit Code + Coin ; A découvre Exit et le besoin de Key. | lecture/local Knowledge, pas de Quest Graph magique |
| M3 | P prend deux Coins ; A découvre Safe et Energy + Coin + Battery. | Safe fermé ne révèle pas Key |
| M4–M5 | P informe B ; B relie l'information à son Code, sans révélation automatique ; une Proposal/Agreement reste non prescriptive. | communication et social ≠ Action forcée |
| M6–M8 | Ressources distribuées ; P/B activent Generator ; A ouvre Safe, perçoit puis prend Key. | Conditions, exposition, `OPEN SAFE ≠ OWNED KEY`, Inventory Commit |
| M9 | P soigne B ; B sait légitimement que P l'a soigné. | Recovery/interaction ≠ Trust/coopération imposés |
| M10–M11 | A possède Key et connaît déjà Exit ; A ouvre Exit puis CrossExit START. | `HAS KEY ≠ KNOWS EXIT`, `OPEN EXIT ≠ ESCAPED` |
| M12 | Broadcast, Alarm et displays informent légitimement P/B de l'évacuation. | pas de connaissance magique de localisation ou timer |
| M13–M15 | P/B évaluent leurs options sous pression ; P sort, puis Timer expire avant B. | Appraisal non prescriptif, outcomes Cooperative, END ATTEMPT |

## Contrôles validés

| Contrôle | Résultat |
|---|---|
| NPC lit directement le Quest Graph ? | NO / PASS |
| Objet réservé par un Plan ? | NO / PASS |
| B donne automatiquement le Code ou après Heal ? | NO / PASS |
| Agreement impose une Action ? | NO / PASS |
| Changement World produit Knowledge universelle ? | NO / PASS |
| Key possédée implique connaissance de l'Exit ? | NO / PASS |
| Appraisal ou psycho-social impose une solution ? | NO / PASS |
| Action contourne les systèmes World autoritatifs ? | NO / PASS |
| Exit OPEN implique ESCAPED ? | NO / PASS |
| NPC connaît magiquement le Timer ? | NO / PASS |
| Exit connaît Game Mode ? | NO / PASS |
| END ATTEMPT est confondu avec état participant ? | NO / PASS |

**Résultat :** `NEW STRUCTURAL BLOCKER = NONE`.

## Conclusion de référence

> Même problème initial + participants autonomes + connaissances partielles + ressources limitées + interactions sociales → histoires différentes mais causalement explicables.

GST01 devient une référence de non-régression conceptuelle : toute évolution doit préserver l'absence d'accès illégitime au Quest Graph, de réservation magique, de causalité sociale scriptée et de rupture de parité Player/NPC.

L'alarme et le temps restant peuvent signaler l'urgence sans révéler la localisation de l'Exit. Toute signalétique additionnelle reste une future décision de Level/Information Design.
