# EXIT Design Bible
## 33 - Registre des décisions et changelog

**Statut :** 🟡 WIP  
**Dernière mise à jour :** 17 septembre 2026

## Règle

Une décision validée doit indiquer sa date, son statut, sa raison, son impact et les éléments qu'elle remplace. Une proposition ne devient jamais canonique par silence.

| Version | Date | Évolution | Statut |
|---|---|---|---|
| Audit 0.1 | 16 septembre 2026 | Sources inventoriées, concepts séparés, contradictions et backlog établis. | 🟡 |
| 01 v0.1 | 16 septembre 2026 | Vision, identité et critères du vertical slice documentés. | 🟡 |
| 07-29 v0.1 | 17 septembre 2026 | Systèmes de monde, puzzle, agents, social, survie, espace, UX et exemples ajoutés. | 🟡 |
| 30-33 v0.1 | 17 septembre 2026 | Architecture conceptuelle, glossaire, backlog et registre ajoutés. | 🟡 |
| DR-01 | 17 septembre 2026 | Vision, identité et piliers validés par Nicolas + ChatGPT. | 🟢 CANON |
| DR-02 | 17 septembre 2026 | Vocabulaire, tableau noir IA et frontières lexicales validés par Nicolas + ChatGPT. | 🟢 CANON |
| DR-03 | 17 septembre 2026 | Frontières Gestionnaire de quêtes / Gestionnaire de partie / Monde / IA NPC validées par Nicolas + ChatGPT. | 🟢 CANON |
| DR-04 | 18 septembre 2026 | Modèle Agent, mémoire et grammaire systémique validés par Nicolas + ChatGPT. | 🟢 CANON |
| DR-05 | 19 septembre 2026 | Modèle psycho-émotionnel minimal et social consolidé par Nicolas + ChatGPT. | 🟢 CANON |
| DR-06 | 20 septembre 2026 | Modèle de décision subjectif et friction décisionnelle validés. | 🟢 CANON |
| DR-07 | 20 septembre 2026 | Contrat d'échec, relance, Observers et débrief validés. | 🟢 CANON |
| DR-08 (01-05) | 21 septembre 2026 | Vertical slice de référence, accès subjectif et communication validés. | 🟢 CANON |
| DR-08-06 | 21 septembre 2026 | Communication et engagements sociaux à l'exécution validés. | 🟢 CANON |
| DR-08-07 | 22 septembre 2026 | Runtime d'appréciation subjective et de décision NPC validé. | 🟢 CANON |

## DR-08-07 — décisions validées

| ID | Décision | Statut |
|---|---|---|
| DR08-07 D01 à D12 | Décisions détaillées dans `EXIT_DESIGN_REVIEW_DR08_07_NPC_DECISION_APPRAISAL_RUNTIME.md`. | 🟢 CANON |

## DR-08-06 — décisions validées

| ID | Décision | Statut |
|---|---|---|
| DR08-06 D01 à D11 | Décisions détaillées dans `EXIT_DESIGN_REVIEW_DR08_06_COMMUNICATION_SOCIAL_COMMITMENT_RUNTIME.md`. | 🟢 CANON |

## DR-08 — décisions validées

| ID | Décision | Statut |
|---|---|---|
| DR08-01 à DR08-04 | Accès subjectif, slice, état initial et topologie. | 🟢 CANON |
| DR08-05 D01 à D08 | Exposé/interne, transitions, concurrence, communication, affirmations et accords sociaux. | 🟢 CANON |

## DR-07 — décisions validées

| ID | Décision | Statut |
|---|---|---|
| DR07-D01 à D10 | Décisions détaillées dans `EXIT_DESIGN_REVIEW_DR07_FAILURE_SOLVABILITY_OBSERVERS_DEBRIEF.md`. | 🟢 CANON |

## DR-06 — décisions validées

| ID | Décision | Statut |
|---|---|---|
| DR06-D01 à D15 | Décisions détaillées dans `EXIT_DESIGN_REVIEW_DR06_DECISION_MODEL.md`. | 🟢 CANON |

## DR-05 — décisions validées

| ID | Décision | Statut | Conséquence principale |
|---|---|---|---|
| DR05-D01 | Profil psycho-émotionnel interne. | 🟢 CANON | État et modulateurs, jamais décision directe. |
| DR05-D02 | État central, stress réactif, réactions transitoires. | 🟢 CANON | Sécurité ressentie et confiance en soi sont initiales. |
| DR05-D03 | Réassurance = influences, pas jauge. | 🟢 CANON | Monde, connaissance, social et soi peuvent apaiser. |
| DR05-D04 | Empreinte émotionnelle neutre en valence. | 🟢 CANON | Une expérience peut compter après disparition de l'émotion. |
| DR05-D05 | Empreinte intégrée à la mémoire. | 🟢 CANON | Pas de base émotionnelle parallèle. |
| DR05-D06 | Appréciation avant émotion. | 🟢 CANON | Événement ≠ modificateur direct. |
| DR05-D07 | Confiance en soi retenue, espoir non persistant. | 🟢 CANON | Évolution située, pas de bonus universel. |
| DR05-D08 | Facteurs historiques = références. | 🟢 CANON | Pas de promotion automatique en jauges. |
| DR05-D09 | États modulent, DR-06 décide. | 🟢 CANON | État ≠ décision. |
| DR05-D10 | Événements sociaux évalués par systèmes IA. | 🟢 CANON | Relation/confiance non mises à jour directement. |

## DR-04 — décisions validées

| ID | Décision | Statut | Conséquence principale |
|---|---|---|---|
| DR04-D01 | Tableau noir privé = état exploitable ; systèmes IA = traitements. | 🟢 CANON | Cerveau fonctionnel = systèmes + tableau noir. |
| DR04-D02 | Mémoire = banque sélective de connaissances. | 🟢 CANON | Ni snapshot, ni journal, ni pipeline obligatoire. |
| DR04-D03 | Source recommandée ; mode d'acquisition distinct. | 🟢 CANON | Source peut être inconnue ou absente. |
| DR04-D04 | Acquisition qualifiée avant consolidation. | 🟢 CANON | Perception/observation ≠ connaissance automatique. |
| DR04-D05 | Politique de mémorisation légère et gameplay. | 🟢 CANON | Pas d'ontologie exhaustive. |
| DR04-D06 | Consolidation CREATE / IGNORE / VERSION. | 🟢 CANON | Une version CURRENT est privilégiée. |
| DR04-D07 | Timestamp = acquisition de l'état pertinent. | 🟢 CANON | Répétition identique n'écrit pas. |
| DR04-D08 | Fiabilité subjective possible, sans renforcement par répétition. | 🟢 CANON (principe) | Calcul détaillé WIP. |
| DR04-D09 | Seuls états sémantiques gameplay sont mémorisés. | 🟢 CANON | Pas de bruit technique continu. |
| DR04-D10 | Changement temporel ≠ contradiction. | 🟢 CANON | Historique valide, CURRENT pertinente. |
| DR04-D11 | Inventaire physique distinct de mémoire/connaissance. | 🟢 CANON | Pas de fiabilité par objet porté. |
| DR04-D12 | Capacités fondamentales communes + grammaire systémique. | 🟢 CANON | Divergence par contexte et état. |
| DR04-D13 | État → évaluation IA → besoin. | 🟢 CANON | État ≠ besoin ≠ but ≠ action. |
| DR04-D14 | Inférence non requise au modèle de base. | 🔵 PROPOSITION | À justifier par cas gameplay. |

## DR-03 — décisions validées

| ID | Question | Décision | Statut | Conséquence principale | Documents impactés |
|---|---|---|---|---|---|
| DR03-D01 | Qui instancie ? | Quête décrit, partie orchestre, Monde instancie. | 🟢 CANON | Séparation problème/session/simulation. | 15, 30, 31 |
| DR03-D02 | Qui évalue les conditions ? | Partie évalue graphe + état autoritatif. | 🟢 CANON | La logique n'est pas réinventée à l'exécution. | 15, 30 |
| DR03-D03 | Le graphe change-t-il ? | Immuable par défaut ; correspondance d'exécution évolutive. | 🟢 CANON | Monde dynamique sans mutation implicite du problème. | 15, 30, 31 |
| DR03-D04 | Le Monde lit-il le tableau noir ? | Non ; les systèmes IA médiatisent Monde et tableau noir. | 🟢 CANON | Vie privée conceptuelle de l'agent. | 08, 18, 30 |
| DR03-D05 | Une lecture IA vaut-elle connaissance ? | Non ; perception/cognition déterminent l'acquisition. | 🟢 CANON | Évite l'omniscience technique. | 11, 18, 30 |
| DR03-D06 | Une branche perdue vaut-elle défaite ? | Non automatiquement ; règles/mode/Failure Contract donnent le sens. | 🟢 CANON | Report du contrat d'échec à DR-07. | 15, 30, 32 |
| DR03-D07 | Quête connaît-elle les NPC ? | Exigences abstraites seulement ; partie lie les instances. | 🟢 CANON | Le Gestionnaire de quêtes ne devient pas cerveau des NPC. | 15, 18, 30 |
| DR03-D08 | Comment surveiller les conditions ? | Événements pertinents + contrôle périodique basse fréquence. | 🟢 CANON | 400 ms n'est pas canonique. | 30, 32 |

## DR-02 — décisions validées

| ID | Question | Décision | Statut | Raison et conséquences | Documents impactés | Remplace |
|---|---|---|---|---|---|---|
| DR02-D01 | Où résident la réalité simulée et l'état local NPC ? | Monde/état du monde autoritatif ; tableau noir IA privé pour l'état interne. | 🟢 CANON | L'état local peut être subjectif sur le Monde. | 08, 18, 30, 31 | Frontière WIP DR-02 |
| DR02-D02 | Comment distinguer perception et observation ? | Signal reçu localement / acquisition située exploitable. | 🟢 CANON | Aucune ne donne la vérité globale. | 11, 31 | — |
| DR02-D03 | Que sont information, mémoire, connaissance et inférence ? | Contenu / trace contextualisée / représentation locale révisable / processus de révision. | 🟢 CANON | Mémoire et connaissance interagissent, sans chaîne obligatoire. | 11, 31 | Modèle séquentiel WIP |
| DR02-D04 | Faut-il une croyance indépendante ? | Non, pas à ce stade. | 🟢 CANON | Une connaissance peut être incorrecte ou incertaine. | 11, 31 | Proposition de croyance indépendante |
| DR02-D05 | Quelle différence entre besoin et but ? | Pression modulatrice / état désiré poursuivi. | 🟢 CANON | État ≠ décision. | 18, 20, 31 | — |
| DR02-D06 | Comment structurer le problème ? | Quête → Objectifs → Conditions ; tâches historiques. | 🟢 CANON | Le Gestionnaire de quêtes construit le problème, les agents cherchent la résolution. | 14, 15, 31 | Quête → Objectifs → Tâches |
| DR02-D07 | Comment nommer le raisonnement agent ? | Stratégie → Plan → Actions ; pas de tâche intermédiaire. | 🟢 CANON | N'impose pas GOAP. | 14, 19, 31 | Hiérarchie WIP antérieure |
| DR02-D08 | Comment distinguer états psychologiques ? | Personnalité / état psychologique / émotion séparés. | 🟢 CANON | Aucun ne dicte seul une action. | 20, 31 | — |
| DR02-D09 | Comment nommer relation et confiance ? | Relation directionnelle ; confiance = estimation contextuelle de fiabilité. | 🟢 CANON | Confiance ≠ acceptation ou seuil comportemental. | 21, 31 | — |

## DR-01 — décisions validées

| ID | question | Décision | Statut | Raison et conséquences | Documents impactés | Remplace |
|---|---|---|---|---|---|---|
| DR01-D01 | Quelle est la définition d'EXIT ? | Jeu d'évasion systémique multi-agents : le joueur cherche une voie vers la sortie en agissant sur un réseau dynamique de dépendances. | 🟢 CANON | Fonde l'identité ; ne garantit pas encore une situation gagnable. | 01, 02-06, 31 | Formulations antérieures WIP |
| DR01-D02 | Que sont les NPC ? | Agents autonomes, situés, à connaissance partielle ; pas de vérité globale automatique. | 🟢 CANON | Autonomie n'est ni omniscience ni simulation humaine exhaustive. | 01, 31 | — |
| DR01-D03 | Quel rôle a l'information ? | Ressource systémique : savoir, croire, découvrir, déduire ou apprendre modifie les voies accessibles. | 🟢 CANON | Les définitions fines relèvent de DR-02. | 01, 02-06, 31 | — |
| DR01-D04 | Quelle place a la coopération ? | Facultative, avantageuse ou nécessaire si la nécessité provient d'une contrainte intelligible, jamais d'un verrou social arbitraire. | 🟢 CANON | Autorise une action à deux ; rejette confiance-seuil => refus comme modèle général. | 01, 21, 32 | « jamais obligatoire » |
| DR01-D05 | Quelle structure porte l'évasion ? | Réseau de dépendances entre états, lieux, objets, ressources, machines, informations et agents. | 🟢 CANON | N'impose ni procéduralité, ni AND/OR technique, ni anti-softlock. | 01, 02-06, 31 | — |
| DR01-D06 | Quel contrat de feedback ? | Causes et conséquences pertinentes interprétables depuis les informations accessibles, sans divulgation obligatoire de la solution exacte. | 🟢 CANON | Protège déduction et équité sans sur-guidage. | 01 | — |
| DR01-D07 | Quelle boucle joueur ? | Observer → Comprendre → Décider → Agir → Réévaluer. | 🟢 CANON | Boucle d'expérience, non architecture NPC ou moteur. | 01 | — |
| DR01-D08 | Que ne décide pas DR-01 ? | Procéduralité, caméra, temps, survie, psychologie, trust, dangers, violence, mortalité, GOAP, slice et architecture restent hors périmètre. | 🟢 CANON | Empêche une canonisation implicite par l'historique. | 01, 31, 32 | — |

## Décisions en attente

- Valider la frontière Gestionnaire de quêtes / agent / GOAP.
- Valider le vocabulaire connaissance/mémoire, but/objectif, tâche/action et confiance.
- Choisir temporalité, mortalité, conflit et profondeur de simulation psychologique.
