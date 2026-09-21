# EXIT — Revue de conception DR-01
## Vision, identité et piliers

**Statut :** 🟢 Revue clôturée  
**Date :** 17 septembre 2026  
**Participants :** Nicolas, ChatGPT  
**Responsabilité de décision :** Nicolas est le seul directeur de jeu. ChatGPT intervient comme responsable de conception et architecte documentaire.

## 1. Objet et périmètre

DR-01 stabilise l'identité de conception d'EXIT. Elle définit la cible de conception — ce que le jeu doit être — sans valider une tranche verticale, une architecture d'implémentation, un algorithme de planification ou un modèle détaillé d'agent.

Sont hors périmètre : procéduralité, perspective, temporalité, survie, psychologie détaillée, relations, confiance, dangers, violence, mortalité, GOAP détaillé, tranche verticale et architecture Olympe Engine.

## 2. Décisions validées

### DR01-D01 — Définition d'EXIT

**🟢 CANON**

> EXIT est un jeu d'évasion systémique multi-agents dans lequel le joueur cherche une voie vers la sortie en comprenant et en agissant sur un réseau dynamique de dépendances, au sein d'un monde peuplé d'autres agents autonomes confrontés au même problème.

« Confrontés au même problème » ne signifie ni mêmes connaissances, ni mêmes objectifs immédiats, stratégies, intérêts ou capacités. « Cherche une voie » ne garantit pas encore que toute situation soit gagnable ; ce contrat relève de DR-07.

### DR01-D02 — NPC autonomes

**🟢 CANON**

> Les NPC sont des agents autonomes confrontés au même monde-problème que le joueur. Ils agissent à partir de leur état local, de leurs capacités et de leur connaissance partielle du monde ; ils ne disposent pas automatiquement de la vérité globale du niveau.

L'autonomie n'implique ni omniscience, ni simulation humaine exhaustive, ni identité parfaite des capacités ou de l'interface du joueur.

### DR01-D03 — Information comme ressource systémique

**🟢 CANON**

> L'information est une ressource systémique : ce qu'un agent sait, croit, découvre, déduit ou apprend peut modifier les décisions et les voies de résolution qui lui sont accessibles.

Cette décision valide le principe, non les définitions précises d'information, connaissance, croyance, mémoire, inférence, certitude, source, fraîcheur ou vérification. Celles-ci relèvent de DR-02 puis du modèle Agent.

### DR01-D04 — Coopération systémique

**🟢 CANON**

> La coopération est une stratégie systémique résultant de la situation et des agents. Elle peut être facultative, avantageuse ou nécessaire. Lorsqu'elle est nécessaire, cette nécessité doit découler des contraintes intelligibles du problème et non d'un verrou social arbitraire.

Une porte qui exige réellement deux opérateurs est une coopération nécessaire valide. Trouver, aider, convaincre, négocier ou aligner temporairement les intérêts d'un autre agent peut alors constituer une voie de résolution. Une formule du type « confiance inférieure à un seuil, donc refus » est rejetée comme principe général.

### DR01-D05 — Réseau de dépendances

**🟢 CANON**

> Le problème d'évasion d'EXIT est structuré par des dépendances entre états du monde, lieux, objets, ressources, machines, informations et agents. Les puzzles locaux peuvent exister, mais prennent leur sens principal par leur contribution à cette situation globale.

Ce pilier n'impose ni génération procédurale, ni représentation technique ET/OU, ni pluralité obligatoire des solutions, ni anti-blocage permanent.

### DR01-D06 — Retour d'information interprétable

**🟢 CANON**

> Les causes et conséquences pertinentes doivent être interprétables à partir des informations accessibles au joueur, sans obligation de révéler la précondition exacte ni la solution.

Un mécanisme qui ne répond pas avec voyant d'alimentation éteint constitue un retour lisible ; le jeu n'a pas à révéler l'emplacement précis de la batterie. Le joueur observe, expérimente et déduit.

### DR01-D07 — Boucle d'expérience joueur

**🟢 CANON**

```text
OBSERVER → COMPRENDRE → DÉCIDER → AGIR → RÉÉVALUER → OBSERVER
```

Il s'agit d'une boucle d'expérience joueur. Elle ne définit pas une architecture cognitive des NPC, un arbre de comportements, GOAP, des classes moteur ou une séquence logicielle obligatoire. EXIT récompense principalement la compréhension et la prise de décision, pas seulement l'exécution mécanique.

### DR01-D08 — Frontière de la revue

**🟢 CANON**

La présence historique ou la probabilité future d'un mécanisme hors périmètre ne vaut pas validation. Les décisions ultérieures devront être prises dans leur revue dédiée.

## 3. Candidat enregistré, non validé

### Non-déterminisme par seuil

**🟡 Candidat — à examiner en DR-05 et DR-06**

> Les variables internes et relationnelles modulent l'évaluation des options mais ne déterminent pas seules une décision. Le contexte, l'urgence, les besoins, les alternatives et les conséquences anticipées peuvent conduire un agent à agir malgré un état relationnel ou psychologique défavorable.

L'intention est : **état ≠ décision**. Cas de test : incendie sans route sûre ; blessure critique sans alternative ; alliance circonstancielle de deux survivants devant accomplir une action à deux. Accepter une aide ou coopérer dans l'urgence ne signifie pas automatiquement faire confiance.

Hypothèse de travail non technique :

```text
Monde / situation → contexte
                       ↓
personnalité + état interne + relation
                       ↓
appréciation contextuelle → but / stratégie → planification → action
```

## 4. Documents impactés

La propagation contrôlée de DR-01 concerne les documents 01, 02-06, 21, 31, 32 et 33. Les sections 11 à 30 ne sont pas réécrites pour forcer une compatibilité : toute tension reste signalée jusqu'à sa revue dédiée.

## 5. Registre de la revue

| Champ | Valeur |
|---|---|
| Décisions approuvées | DR01-D01 à DR01-D08 |
| Candidat enregistré | Non-déterminisme par seuil — 🟡 |
| Documents de suivi | Propagation contrôlée ; DR-02 Vocabulaire canonique |
| Registre des décisions | Section 33 — DR01-D01 à DR01-D08 |

