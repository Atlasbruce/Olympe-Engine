# EXIT — Revue de conception DR-06
## Modèle de décision

**Statut :** 🟢 REVIEW COMPLETED  
**Date :** 20 septembre 2026  
**Principe :** un NPC ne recherche pas une action objectivement optimale à partir de la vérité globale ; il apprécie subjectivement les possibilités qu'il connaît et estime possibles.

## 1. Modèle conceptuel

```text
Situation → besoins / buts / opportunités → options connues
→ rappel contextuel → appréciation des options → filtrer / comparer
→ décision → but / stratégie → plan → actions → changement du Monde → réévaluation
```

Ce modèle ne prescrit ni algorithme, ni score unique, ni formule de pondération.

## 2. Décisions validées

| ID | Décision | Statut |
|---|---|---|
| D01 | Une possibilité dans le Monde, connue du NPC, perçue faisable et subjectivement acceptable sont quatre notions distinctes. | 🟢 CANON |
| D02 | But = résultat recherché ; stratégie = approche générale ; plan = séquence concrète d'actions. | 🟢 CANON |
| D03 | Une option porte un vecteur d'appréciation multidimensionnel, non un score unique imposé. | 🟢 CANON |
| D04 | Importance et urgence sont distinctes ; une opportunité peut être mémorisée sans devenir prioritaire. | 🟢 CANON |
| D05 | Le rappel contextuel fournit connaissances, expériences et empreintes pertinentes sans balayer toute la mémoire ni décider l'action. | 🟢 CANON |
| D06 | La mémoire utilise des associations structurées — NPC, lieu, objet, événement, interaction, besoin, but, situation, relation, importance émotionnelle — plutôt que de simples mots-clés. | 🟢 CANON |
| D07 | Capacité objective, état physique et confiance en soi contribuent à la faisabilité perçue, sans interdire/forcer automatiquement une tentative. | 🟢 CANON |
| D08 | Les valeurs morales sont stables ou lentement évolutives, distinctes de la personnalité et du profil psycho-émotionnel. | 🟢 CANON |
| D09 | Rappel d'une trahison, relation, confiance et valeurs peuvent converger ou entrer en conflit ; aucun ne prescrit seul une action. | 🟢 CANON |
| D10 | Les NPC connaissent les règles/mode de jeu pertinents ; le mode coopératif/compétitif peut modifier valeur stratégique et conséquences attendues. | 🟢 CANON |
| D11 | Le stress agit sur la temporalité du traitement, pas sur les capacités fondamentales, les options ni une irrationalité artificielle. | 🟢 CANON |
| D12 | La friction décisionnelle est la difficulté émergente d'un arbitrage entre options viables et dimensions importantes ; elle est distincte du stress. | 🟢 CANON |
| D13 | Une forte friction peut créer une latence ou hésitation visible, jamais automatiquement freeze/panique. | 🟢 CANON |
| D14 | Les options sont distinguées : impossibles, non pertinentes, subjectivement inacceptables/défavorisées, viables ; aucun seuil universel. | 🟢 CANON |
| D15 | Une découverte pertinente peut ajouter ou modifier une option sans interrompre automatiquement le but courant. | 🟢 CANON |

## 3. Vecteur d'appréciation

Les dimensions sont des questions conceptuelles, pas la validation de dix variables numériques : bénéfice attendu, importance, urgence/criticité du besoin, risque perçu, coût perçu, faisabilité perçue, fiabilité des connaissances, valeur stratégique, valeur sociale/relationnelle, alignement moral/valeurs.

## 4. Stress-test de référence

Même Monde : sortie ouverte, mode compétitif, B blessé, décideur avec l'unique medikit.

- A : relation forte, historique d'aide, réciprocité/loyauté élevées ; sauvetage ou compromis peut être plausible.
- B' : relation neutre ; sortie immédiate peut être plausible.
- C : blessé, trahi/abandonné auparavant, stress élevé ; après une hésitation forte, sortie immédiate peut être plausible.

Ces résultats ne sont pas des règles. Aucun n'est « plus rationnel » que les autres : ils sont cohérents avec des appréciations subjectives différentes.

## 5. Trace de décision — piste future

**🟡 WIP.** Une trace structurée pourrait conserver situation, rappel contextuel, options, facteurs saillants, options écartées/retenues, friction, décision, stratégie/plan/actions et réévaluations. Un futur journal intérieur serait une projection lisible de cette trace, non le mécanisme de décision.

## 6. WIP résiduel

- dimensions, fusions, échelles et représentation du vecteur ;
- arbitrage/comparaison, friction, latence et influence quantitative du stress ;
- taxonomie morale ; génération des options connues ; rappel contextuel et associations ;
- inertie du but et déclencheurs de réévaluation.

## 7. Contradictions historiques reclassées

Les anciens seuils, pondérations, chaînes directes émotion→comportement, confiance→refus et modèles d'option « optimale » sont 🟠 HISTORIQUE / DESIGN REFERENCE, jamais un modèle cible.

## 8. Conclusion de revue

**REVIEW COMPLETED.** Aucun blocage conceptuel n'empêche la documentation du modèle. Les paramètres, algorithmes et valeurs listés ci-dessus attendent tests et décisions ultérieures. DR‑07 n'est pas ouvert par cette revue.

