# EXIT Design Bible
## 32 - Design backlog

**Statut :** 🟡 WIP  
**Dernière mise à jour :** 17 septembre 2026

| ID | question | Priorité | Dépendances | Statut |
|---|---|---|---|---|
| DB-01 | Définir le contenu exact du premier vertical slice. | Haute | Vision, graphes, NPC | 🔴 |
| DB-02 | Choisir le modèle temporel. | Haute | État du monde, pression, GOAP | 🔴 |
| DB-03 | Définir incapacité, mort et récupération joueur/NPC. | Haute | Solvabilité, survie | 🔴 |
| DB-04 | Sélectionner les Actions du prototype. | Haute | tâche/action, UI | 🔴 |
| DB-05 | Définir le noyau personality/psychologie/relations. | Haute | NPC, GOAP | 🔴 |
| DB-06 | Définir les règles de mensonge, vol et conflit. | Moyenne | Dialogue, relations | 🔴 |
| DB-07 | Valider les invariants anti-softlock et leur coût. | Haute | Gestionnaire de quêtes | 🔴 |
| DB-08 | Définir la procéduralité du premier acte. | Haute | Gestionnaire de quêtes, level design | 🔴 |
| DB-09 | Déterminer les variables de survie après santé et faim. | Moyenne | Pressure loop | 🔴 |
| DB-10 | Fixer le canon narratif du complexe et de la sortie. | Moyenne | Lore, art | 🔴 |
| DB-11 | Définir le journal, la carte et la visibilité sociale. | Moyenne | connaissance, UI | 🔴 |
| DB-12 | Examiner le principe candidat « non-déterminisme par seuil » : les états internes et relationnels modulent l'évaluation sans décider seuls. | Haute | DR-05, DR-06 | 🟡 |
| DB-13 | Distinguer la fiabilité subjective d'une connaissance et sa validité temporelle actuelle. | Haute | DR-04, modèle Agent | 🟡 |
| DB-14 | Définir la politique de lecture/écriture, d'événements et de satisfaisabilité entre Gestionnaire de quêtes, Gestionnaire de partie, Monde et IA NPC. | Haute | DR-03 | 🔴 |
| DB-15 | Définir le modèle Agent : tableau noir, mémoire, connaissance, inventaire, capacités et flux d'information. | Haute | DR-04 | 🔴 |
| DB-16 | Définir le Failure Contract après constat d'une branche irréalisable. | Haute | DR-07 | 🔴 |
| DB-17 | Définir les règles et le calcul de fiabilité subjective des connaissances. | Moyenne | DR-05/06, tests | 🟡 |
| DB-18 | Tester puis enrichir seulement si nécessaire la taxonomie légère de connaissances. | Moyenne | DR-04, tests | 🟡 |
| DB-19 | Justifier par un cas gameplay l'ajout éventuel d'un mécanisme d'inférence. | Basse | DR-06, tests | 🔵 |
| DB-20 | Définir les valeurs, courbes et vitesses de régulation psycho-émotionnelle. | Moyenne | Tests | 🟡 |
| DB-21 | Définir les critères d'admission d'une empreinte émotionnelle en mémoire. | Moyenne | Tests | 🟡 |
| DB-22 | Réévaluer l'ajout de dimensions psycho-émotionnelles seulement si un cas gameplay le justifie. | Basse | Tests | 🟠 |
| DB-23 | Définir algorithme, échelles et représentation de l'appréciation multidimensionnelle des options. | Haute | Tests décisionnels | 🟡 |
| DB-24 | Définir le calcul de friction et latence décisionnelles. | Moyenne | Tests décisionnels | 🟡 |
| DB-25 | Définir la taxonomie minimale des valeurs morales. | Moyenne | Tests sociaux | 🟡 |
| DB-26 | Étudier une trace de décision structurée pour récit systémique et débogage. | Basse | Outils, UI | 🔵 |
| DB-27 | Étudier un évaluateur global/récursif de solvabilité et d'atteignabilité. | Haute | Tests, architecture | 🟡 |
| DB-28 | Définir modèle décisionnel des Observers et vote du public. | Moyenne | Observers | 🟡 |
| DB-29 | Définir valeurs des timers, régulation au restart et UI de mort/restart/reload. | Moyenne | UI, tests | 🟡 |
| DB-30 | Étudier convergence future trace d'événements / trace de décision. | Basse | Debrief | 🔵 |

## Candidat issu de DR-01

### DB-12 — Non-déterminisme par seuil

**Formulation de travail :** les variables internes et relationnelles modulent l'évaluation des options, mais ne déterminent pas seules une décision. Le contexte, l'urgence, les besoins, les alternatives et les conséquences anticipées peuvent conduire un agent à agir malgré un état relationnel ou psychologique défavorable.

**Intention à préserver :** `ÉTAT ≠ DÉCISION`. Le modèle général « confiance(A→B) inférieur à un seuil, donc refus » est rejeté comme verrou social opaque. Une alliance circonstancielle ou l'acceptation d'une aide ne signifie pas automatiquement une relation de confiance.

**Cas de test à reprendre en DR-05 / DR-06 :** incendie sans route sûre ; blessure critique sans alternative ; deux derniers survivants devant accomplir une action à deux.
