# EXIT — Revue de conception DR-05
## Modèle psychologique, émotionnel et social

**Statut :** 🟢 REVIEW COMPLETED  
**Date :** 19 septembre 2026  
**Participants :** Nicolas, ChatGPT  
**Principe directeur :** `ÉTAT / TRAIT / RELATION ≠ DÉCISION`.

## 1. Modèle psycho-émotionnel canonique

Le **profil psycho-émotionnel** est la configuration interne propre à chaque NPC qui influence la manière dont il ressent, interprète et évalue les situations. Il réunit l'état psycho-émotionnel central, les réactions actives, le stress, la personnalité, les besoins, mémoire/connaissance, relations/confiance et expériences émotionnelles mémorisées. Il ne décide jamais directement d'une action : il participe à une appréciation subjective que DR‑06 exploitera.

```text
Monde / événements → perception → mémoire / connaissance
                         ↓
       besoins + personnalité + relation / confiance
                         ↓
             appréciation psycho-émotionnelle
                         ↓
┌──────────────────────────────────────────────┐
│ État central : sécurité ressentie, confiance │
│ en soi                                       │
├──────────────────────────────────────────────┤
│ État réactif : stress                         │
├──────────────────────────────────────────────┤
│ Réactions temporaires : peur, colère, joie…  │
└──────────────────────────────────────────────┘
                         ↓ régulation / expérience
          expérience significative ? → empreinte émotionnelle → mémoire
                                                   ↺ appréciation future
```

Ce diagramme décrit des interactions conceptuelles ; ce n'est ni un pipeline technique obligatoire ni un modèle de décision.

## 2. Décisions validées

### DR05-D01 — Profil psycho-émotionnel

**🟢 CANON.** Le profil psycho-émotionnel est la configuration interne d'un NPC qui influence son ressenti, son interprétation et son appréciation subjective. Il n'est pas une décision ni une liste fermée de jauges.

### DR05-D02 — Trois temporalités

**🟢 CANON.**

- **État psycho-émotionnel central, persistant mais évolutif :** sécurité ressentie et confiance en soi.
- **État réactif avec inertie :** stress, qui s'accumule, persiste et se régule progressivement.
- **Réactions émotionnelles transitoires :** peur, colère, tristesse, joie, surprise, dégoût, etc., situées et temporaires.

La **sécurité ressentie** est le sentiment global, subjectif et intrinsèque de sécurité : ni danger objectif, ni probabilité de survie. La **confiance en soi** est le sentiment de pouvoir comprendre, affronter et surmonter les difficultés : elle est distincte de la capacité réellement disponible. Il n'y a pas de couple Baseline/Current à ce stade.

Chaque réaction peut avoir intensité, déclencheur/contexte, instant de déclenchement et profil de régulation. Aucun délai de retour au calme universel n'est défini ; valeurs, courbes et vitesses sont 🟡 WIP.

### DR05-D03 — Réassurance

**🟢 CANON.** La réassurance n'est pas une jauge persistante ; c'est une famille d'influences positives sur la régulation psycho-émotionnelle. Elle peut venir du Monde (abri, ressources), de la connaissance (comprendre, savoir quoi faire), du social (aide, coopération, présence fiable) ou de soi (contrôle, capacité).

> Être rassuré dans EXIT, c'est percevoir que l'on possède — soi-même, dans le monde, dans ses connaissances ou chez les autres — des moyens permettant de comprendre, contrôler ou affronter la situation.

### DR05-D04 / D05 — Empreinte émotionnelle et mémoire

**🟢 CANON.** Une **empreinte émotionnelle** est la trace mémorisée et contextualisée d'une expérience émotionnellement significative. Elle est neutre en valence : peur intense, soulagement, réussite, aide, trahison, sauvetage ou découverte peuvent tous en laisser une. Le terme « trauma » n'est pas un concept système générique.

Une réaction peut disparaître sans que l'expérience cesse d'avoir des conséquences. L'empreinte est dormante dans la mémoire, potentiellement rappelée par un lieu, NPC, menace, objet, situation analogue ou interaction similaire. Elle participe à une nouvelle appréciation, jamais directement à une action.

Il n'existe pas de base `EmotionalMemory` parallèle : une expérience mémorisée significative peut porter un **contexte émotionnel facultatif**. Toutes les variations ne sont pas enregistrées ; l'admission dépend de la pertinence gameplay. Critères exacts 🟡 WIP.

### DR05-D06 — Appréciation avant émotion

**🟢 CANON.** Aucun événement ne donne directement une émotion chiffrée. Le même acte est interprété selon sécurité ressentie, confiance en soi, stress, personnalité, besoins, mémoire/connaissance, empreintes, relations/confiance et contexte.

```text
Événement / situation → perception → contexte mémoire/connaissance
→ appréciation psycho-émotionnelle → évolution éventuelle de l'état central
→ stress → réaction(s) émotionnelle(s) → empreinte éventuelle → mémoire
```

### DR05-D07 — Confiance en soi et espoir

**🟢 CANON.** La confiance en soi appartient à l'état central et évolue avec l'expérience après appréciation ; aucun bonus/malus universel n'est défini. **L'espoir n'est pas une troisième jauge persistante.** Il peut émerger d'une situation selon possibilités perçues, connaissances, confiance en soi, sécurité ressentie, besoins et contexte. Il peut être réévalué ultérieurement comme concept émotionnel.

### DR05-D08 — Facteurs historiques

**🟢 CANON quant à leur statut.** Clarté, soutien social, contrôle, capacité, espoir, dignité, sens, moralité et autres facteurs historiques restent 🟠 références de conception. Ils ne deviennent pas automatiquement des jauges persistantes. La clarté peut être appréciée depuis mémoire/connaissance ; le soutien social depuis relation/confiance, présence et capacité connue ; le contrôle/capacité peuvent être des dimensions d'appréciation. Le modèle est minimal et extensible seulement si un cas gameplay le justifie.

### DR05-D09 — Relation au modèle de décision

**🟢 CANON.** Peur intense, colère, faible sécurité ressentie ou faible confiance en soi ne déclenchent pas automatiquement fuite, attaque, refuge ou abandon. Ces états modulent l'appréciation des options ; DR‑06 déterminera leur emploi dans but, stratégie, plan et action.

### DR05-D10 — Modèle social

**🟢 CANON.** Les décisions DR‑02 demeurent : `Relation(A→B)` est directionnelle ; `Confiance(A→B)` est une estimation contextuelle de fiabilité/prédictibilité, non une affection, coopération ou acceptation automatique. Les systèmes IA évaluent les événements sociaux avant toute évolution relationnelle. La portée psycho-émotionnelle d'un événement peut contribuer à cette évaluation : recevoir un medikit près de la mort peut être plus significatif que le même don en sécurité.

## 3. Références historiques préservées

Les jauges `EmotionState`, Maslow, Plutchik, les listes Clarity/SocialSupport/Control/Hope/etc. et les matrices de pondération sont **🟠 HISTORIQUE / DESIGN REFERENCE**. Elles documentent l'évolution du projet, sans constituer un objectif d'exécution. Elles mélangeaient souvent faits, besoins, relations, émotions et actions ; ce mélange est explicitement écarté du modèle cible.

## 4. Éléments WIP et reportés

| Élément | Statut | Suite |
|---|---|---|
| Valeurs, courbes, vitesses et règles de régulation | 🟡 WIP | Tests ultérieurs |
| Critères d'admission d'une empreinte émotionnelle | 🟡 WIP | Tests ultérieurs |
| Taxonomie complète des réactions | 🟡 WIP | Seulement si valeur gameplay démontrée |
| Calcul de l'évolution relationnelle et de la confiance | 🟡 WIP | DR‑06 |
| Décision, arbitrage, stratégie et plan | Reporté | DR‑06 |
| Failure Contract et conséquences terminales | Reporté | DR‑07 |

## 5. Registre de revue

| Champ | Valeur |
|---|---|
| Décisions validées | DR05-D01 à DR05-D10 |
| Statut | 🟢 REVIEW COMPLETED |
| Prochaine revue autorisée | DR‑06 seulement après instruction explicite |

## Référence DR-07

Lors d'une relance du niveau, empreintes émotionnelles, relations, confiance et histoire vécue persistent. Le stress est régulé, la confiance en soi restaurée vers un état adapté à une nouvelle tentative et la sécurité ressentie réévaluée. Valeurs et courbes restent WIP.
