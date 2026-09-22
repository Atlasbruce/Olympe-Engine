# EXIT — DR08-06
## Communication et engagements sociaux à l'exécution

**Statut :** 🟢 REVIEW COMPLETED  
**Date :** 21 septembre 2026

## 1. Grammaire de communication

**🟢 CANON.** Un `CommunicationAct` est la grammaire générale :

```text
Speaker + Addressee(s) + Intent + Content + DeliveryMode
```

Elle s'applique aux participants et aux Observers, notamment à leurs broadcasts par haut-parleurs. Les intentions du vertical slice sont `ASK`, `DECLARE`, `REVEAL`, `REQUEST`, `PROPOSE`, `ACCEPT`, `REFUSE`. Une contre-proposition est `REFUSE(P1) + PROPOSE(P2)` ; aucune intention dédiée n'est nécessaire.

Le contenu est sémantique et composable. Un lien de causalité tel que `Reason` doit être communiqué explicitement : deux assertions successives ne créent pas une cause par déduction automatique.

## 2. Communication, proposition et engagement

Un acte de communication, demande, menace ou proposition ne commande jamais directement but, stratégie, plan ou action.

```text
PROPOSAL → ACCEPT → AGREEMENT → COMMITMENT(S)
```

`Agreement / Commitment ≠ Goal ≠ Plan ≠ Action`. Un engagement peut être connu, accepté et actif tout en restant temporairement non exécuté, si une autre priorité prend le plan courant. Plusieurs engagements peuvent coexister ou entrer en conflit : ils participent à l'appréciation et n'ont jamais d'autorité d'exécution propre.

Une proposition sans réponse immédiate est non bloquante. Elle peut être mémorisée et redevenir pertinente selon temps, contexte, relation et confiance ; cet algorithme est 🟡 WIP.

```text
ÉTAT D'ENGAGEMENT ≠ ÉTAT D'EXÉCUTION COURANT
```

## 3. Exécution et réévaluation

```text
événements significatifs → mise à jour de l'état interne
→ demande éventuelle de réévaluation → consolidation des changements
→ appréciation → stratégie → plan → action
```

`MISE À JOUR D'ÉTAT ≠ PRISE DE DÉCISION`. Une réévaluation peut conclure à l'absence de changement et conserver but, stratégie et plan. Plan terminé, impossible ou invalidé retourne à la réévaluation ; l'absence de plan utile est un état valide et réévaluable. Aucun acte, attente de réponse, proposition ou engagement ne crée d'état cognitif bloquant.

## 4. Vérité, information et menace

La vérité objective d'un état social peut différer de ce que savent les participants. Ils raisonnent uniquement à partir des informations auxquelles ils accèdent légitimement.

```text
Menace ≠ capacité prouvée ≠ action future garantie ≠ conformité forcée
```

« Suis-moi ou je te tue » est une communication conditionnelle, non une action forcée ni une conformité imposée.

## 5. Stress-tests de robustesse

**🟠 Illustrations, jamais comportements prescrits.** ST01–ST09 confirment notamment : consolidation d'événements simultanés avant arbitrage ; besoin critique interrompant temporairement un engagement ; proposition sans réponse non bloquante ; engagements concurrents ; mort connue/inconnue ; menace conditionnelle non forcée. Ils testent les frontières du modèle, sans fixer le choix qu'un NPC doit faire.

## 6. WIP

Ton/indices expressifs et taxonomie ; taxonomie exhaustive du contenu ; durée/persistance des propositions ; timers/fréquences ; cycle de vie technique complet des engagements ; interruptibilité détaillée des actions.

## 7. Transition documentaire

DR08-07 est seulement un futur cas d'étude : `Importance ≠ Urgency`. Escape peut être très important mais d'urgence modérée ; un broadcast Observer « La porte se referme dans 30 secondes » peut créer une connaissance, modifier subjectivement l'urgence et demander une réévaluation. Aucun modèle DR08-07 n'est conçu ni canonisé ici.

## 8. Décisions enregistrées

| ID | Décision | Statut |
|---|---|---|
| DR08-06 D01 | CommunicationAct général, applicable aux Observers. | 🟢 CANON |
| D02 | Intentions initiales et contre-proposition composée. | 🟢 CANON |
| D03 | Contenu sémantique, causalité explicitée. | 🟢 CANON |
| D04 | Communication ne commande pas une décision. | 🟢 CANON |
| D05 | Accord/engagement distinct de but/plan/action. | 🟢 CANON |
| D06 | Engagements persistants/concurrents, sans autorité d'exécution. | 🟢 CANON |
| D07 | Propositions non bloquantes. | 🟢 CANON |
| D08 | Consolidation avant appréciation et décision. | 🟢 CANON |
| D09 | Réévaluation sans changement et absence de plan valide. | 🟢 CANON |
| D10 | Vérité sociale subjective et menace non forcée. | 🟢 CANON |
| D11 | Stress-tests comme preuves de robustesse. | 🟢 CANON |

