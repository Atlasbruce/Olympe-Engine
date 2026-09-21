# EXIT Design Bible
## Phase 1 - Audit des sources, cartographie et structure cible

**Version :** 0.1 audit  
**Date :** 16 septembre 2026  
**Statut du document :** 🟡 WIP - document de travail, pas une Bible canonique

## 0. Mandat et méthode

Cette phase ne réécrit pas la Bible. Elle inventorie les sources, sépare les idées d'origine des réponses d'assistant, cartographie les concepts, relève les incohérences et prépare une structure versionnable.

### Convention de statut

| Statut | Sens dans ce document |
|---|---|
| 🟢 CANON | Décision explicitement stable et confirmée par le propriétaire du projet. Aucun élément n'est promu CANON par inférence. |
| 🟡 WIP | Direction de travail récurrente et cohérente, mais dont les règles ou paramètres restent à définir. |
| 🔵 PROPOSITION | Hypothèse ou solution suggérée, à étudier et valider. |
| 🟠 HISTORIQUE | Idée retrouvée dans une source ancienne ; elle reste traçable sans être retenue automatiquement. |
| 🔴 À RÉSOUDRE | Contradiction, ambiguïté, dépendance non définie ou arbitrage requis. |

### Principe de preuve

Une phrase attribuée au joueur dans une source est une **intention historique**. Une réponse de Gemini ou ChatGPT est une **proposition historique**, même lorsqu'elle paraît pertinente. Ce rapport ne transforme aucune de ces propositions en décision actuelle sans validation explicite.

---

# A. Inventaire des sources

| ID | Source, origine et époque | Ce qui a été effectivement audité | Valeur actuelle | Statut |
|---|---|---|---|---|
| S01 | **Infernal Escape.pdf**, export historique d'une conversation Gemini, 57 pages, date non inscrite | Les 57 pages, texte et mise en page. Concept initial, réponses du propriétaire, propositions Gemini, modèle émotionnel, extraits de C++ historique. | Source principale de généalogie. Très riche mais hétérogène : séparer les messages du propriétaire des recommandations Gemini et du code ancien. | 🟠 |
| S02 | **EXIT - The Infernal Escape.pptx**, « Olympe Engine Prototype », 11 diapositives, date non inscrite | Les 11 diapositives et la logique visuelle GOAP. | Source historique concise et fiable pour l'intention de décomposition agent : `Find Exit -> Get key -> ...`. Ne spécifie pas le Gestionnaire de quêtes. | 🟠 |
| S03 | Croquis de dépendances puzzle et HUD, photo manuscrite | Sortie verrouillée, clé, coffre, code, batterie/générateur, NPC, objets et chaînes pointillées. | Preuve visuelle de l'idée fondatrice : le niveau relie objets, machines, connaissances et personnages. | 🟠 |
| S04 | Croquis « blessé, medikit, énergie, EXIT », photo manuscrite | NPC blessé, médical, alimentation, accès, sortie ; schéma de relations entre nœuds. | Confirme que le soin et l'état d'un NPC participent au puzzle, et non au seul décor narratif. | 🟠 |
| S05 | Whiteboard « Infernal Escape - Capabilities », photo manuscrite | Boucle : exploration, découverte/observation, mémorisation, analyse, stratégie/choix, planifier tâches, actions/réalisation, conditions de fin/réévaluation. Verbes notés : déplacement, prendre, déposer, utiliser, parler/échanger, résoudre/actionner. | Source historique directe pour la boucle cognitive et les capacités communes. La frontière exacte entre analyse, stratégie, planification et arbre de comportements reste ouverte. | 🟠 |
| S06 | Conversation ChatGPT référencée « EXIT - Game Design », historique récent | Messages utilisateur sur l'univers, les NPC, confiance, objectifs, Gestionnaire de quêtes et exemples de quêtes. Les messages d'assistant ont été distingués. | Énonce les intentions les plus récentes : NPC agents autonomes, information/objets échangeables, coopération non garantie, graphe de dépendances. | 🟡 |
| S07 | **EXIT_Bible_de_Game_Design.docx**, V0 créée dans ce projet | Document entier : 12 chapitres, 16 tableaux, 50 titres. | Bonne synthèse d'amorçage ; non canonique par mandat. Elle a anticipé certains choix et doit être absorbée comme proposition, non comme source d'autorité. | 🔵 |
| S08 | Présente mission « MISSION MAÎTRE — EXIT DESIGN BIBLE » | Mandat entier. | Cadre de travail actuel, exigences de traçabilité, livraison incrémentale et séparation Gestionnaire de quêtes/Agent. | 🟡 |

## Sources citées mais non disponibles dans le périmètre audité

| Source | Situation | Impact sur l'audit |
|---|---|---|
| Conversation Gemini publique d'origine | Lien cité dans S01, contenu non récupéré indépendamment. Le PDF contient un export partiel, qui a été audité. | Ne pas considérer le PDF comme une retranscription exhaustive du lien. |
| Ancien Quest_Manager C++ complet | Seulement des extraits et descriptions dans S01. | Impossible d'auditer le comportement réel, l'état technique, les dépendances et les bugs. À importer avant toute spécification d'implémentation. |
| Images conceptuelles additionnelles des premières conversations | Certaines photos présentes et inspectées ; pas de catalogue daté ni de métadonnées de version. | Elles peuvent illustrer l'intention mais ne permettent pas d'établir une chronologie fine. |
| Toute décision externe non consignée | Non accessible. | Toute affirmation de « canon » devra être validée dans le futur registre de décisions. |

---

# B. Cartographie des concepts

## B1. Identité, monde et expérience

| Concept | Provenance | Lecture d'audit | Statut proposé |
|---|---|---|---|
| Escape game 2D/isométrique, environnement clos à explorer | S01, S03, S06, S07 | Fil rouge constant. La vue isométrique est explicite dans le mandat et les discussions récentes ; le PDF initial parle surtout de 2D. | 🟡 |
| But apparent : trouver et franchir EXIT | S01, S02, S03, S04, S06 | Le but primaire est stable. La nature réelle de la sortie peut devenir une question narrative. | 🟡 |
| Complexe secret, expérimental, clinique, surveillé | S01, S06, S07 | Labo, base militaire et centre expérimental ont été proposés comme variantes. Les caméras, haut-parleurs et vitres sans tain reviennent explicitement. | 🟡 |
| Captifs/sujets sans explication immédiate | S01, S06 | Fort moteur d'enquête. Amnésie et « programme de réinitialisation » viennent surtout de propositions ultérieures : à ne pas confondre avec l'intention initiale. | 🟡 / 🔵 |
| Puzzles comme épreuves délibérées | S01 | Idée cohérente avec l'installation expérimentale ; détail du responsable et de ses motifs ouvert. | 🟡 |
| Récit environnemental par indices, journaux, enregistrements | S01, S06, S07 | Direction retenue à l'état de principe ; formats, quantité et vérité des indices non définis. | 🟡 |

## B2. Boucles de jeu et expérience recherchée

| Boucle ou facteur | Provenance | Intention documentée | Statut |
|---|---|---|---|
| Explorer -> découvrir/observer -> mémoriser -> analyser -> choisir une stratégie -> planifier -> réaliser -> réévaluer | S05, S06 | Boucle cognitive historique, la plus structurante pour les agents. | 🟡 |
| Explorer -> comprendre -> obtenir -> échanger -> combiner -> agir -> s'échapper | S06, S07 | Reformulation de game loop utile au joueur ; « combiner » et le crafting ne sont pas validés. | 🔵 |
| Faim + santé + temps + dangers + autres agents = pression | S01, S06, S07 | Les contraintes de faim et santé sont explicitement souhaitées ; les autres variables dépendent du niveau. | 🟡 |
| Aha moments et lecture progressive des dépendances | S01, S06, S07 | Fun factor clairement induit par les exemples de puzzles ; à formaliser avec tests de lisibilité. | 🟡 |
| Dilemme humain contre solution optimale | S01, S06, S08 | Ressource utilisable pour soi, pour un autre ou pour un verrou. Très compatible avec la vision, mais équilibrage ouvert. | 🟡 |

## B3. Monde systémique et ingrédients

| Famille | Exemples présents dans les sources | Point d'audit | Statut |
|---|---|---|---|
| Accès | clé, badge, carte, code, serrure, porte, coffre, casier | Base très récurrente. Éviter de limiter le système à des clés de couleur. | 🟡 |
| Énergie | batterie, fusible, générateur, alimentation, interrupteur | Supporte des choix d'usage et des chaînes de réparation. | 🟡 |
| Outils | clé à molette, tournevis, pince, levier, outil de forçage | Taxonomie à stabiliser par tags/capacités plutôt que par exceptions. | 🔵 |
| Médical et survie | medikit, bandage, nourriture, eau, santé, faim | Les objets ont un coût d'opportunité social ou personnel. Soif et fatigue sont historiques/proposés, pas encore retenus. | 🟡 / 🔵 |
| Information | code, emplacement, identité, rumeur, indice, carte, observation | Principe majeur : l'information peut être détenue, mémorisée, échangée ou manipulée. | 🟡 |
| Machines et contrôle | terminal, console, générateur, bouton, levier, pompe, caméra, capteur | Interaction physique et lecture visuelle du monde. La liste est ouverte. | 🟡 |
| Obstacles et hazards | eau, gaz, compression, sentinelles, lasers, mines, barrières, pluie acide, trappes | Banque d'idées historique, beaucoup plus large que le noyau à prototyper. Risque de surenchère. | 🟠 / 🔴 |

## B4. Agents et social simulation

| Concept | Ce que disent les sources | Statut |
|---|---|---|
| NPC autonome | Les NPC explorent, perçoivent, mémorisent, prennent, utilisent, échangent, découvrent et poursuivent leurs propres objectifs. | 🟡 |
| Capacités fondamentales communes joueur/NPC | Whiteboard et discussions : le joueur et le NPC manipulent le même problème, même si UI et contrôle diffèrent. | 🟡 |
| NPC non réduit au donneur de quête | Intention répétée dans S06 et S08. | 🟡 |
| Collaboration, refus, manipulation, trahison | Explicite dans S01 et S06. Le périmètre de la violence, du vol et du mensonge reste à définir. | 🟡 / 🔴 |
| Confiance relationnelle et asymétrique | S01 et S06 posent `confiance(A -> B) != confiance(B -> A)`. Plusieurs dimensions possibles : trust, safety, affinity, gratitude, resentment. | 🟡 |
| Aide contextualisée | La valeur d'un don dépend du besoin, de la rareté, du sacrifice, de la relation et de la personnalité. | 🔵, à tester avant modèle chiffré |
| Personnalité émergente continue | Méfiance, altruisme, prudence, ambition, sociabilité, honnêteté, empathie, curiosité, conservation de soi, tolérance au risque, avidité. | 🔵 |
| État psychologique évolutif | Blessure, perte, sécurité et aides reçues font évoluer le comportement à moyen terme. | 🟡 |

## B5. Connaissance, mémoire, cognition

| Couche | Formulation issue des sources | Problème à résoudre | Statut |
|---|---|---|---|
| État du monde | État réel des objets, portes, NPC, ressources et événements. | Définir la granularité et les règles de visibilité. | 🟡 |
| perception | Ce qu'un agent capte localement : vue, son, message, dialogue, trace. | Définir portée, occlusion, bruit et sources. | 🔵 |
| observation | Acte intentionnel qui interprète ou inspecte une cible. | Distinguer de la perception passive dans les règles. | 🟡 |
| mémoire | Trace d'événement vécu ou constaté : « j'ai trouvé le laboratoire », « Bob m'a agressé ». | Durée, oubli, mémoires contradictoires et mémoire épisodique non définis. | 🟡 |
| connaissance | Proposition exploitable : contenu, source, certitude, date, vérification. | Décider si knowledge est déduit de mémoire ou objet de première classe. | 🟡 |
| Analyse | Confronter l'état connu, les besoins et les dépendances pour produire des options. | Préciser les règles de déduction et les limites d'information. | 🔵 |
| Intention/stratégie | Choix d'une direction : survivre, se soigner, obtenir une clé, coopérer, se protéger. | Définir le rapport entre besoin, goal et stratégie. | 🟡 |
| Planification/GOAP | Production d'un plan d'actions vers un état voulu. | Le GOAP est historiquement agent-side ; à garder séparé du Gestionnaire de quêtes. | 🟡 |

## B6. Quêtes, puzzles et génération

| Concept | Provenance et constat | Statut |
|---|---|---|
| Décomposition récursive de `Find Exit` | S02 : sortie -> clé -> coffre -> mot de passe -> chercher/demander/obtenir -> entrer -> ouvrir. | 🟠, socle historique utile |
| quête / objectif / tâche / action | S06, S07, S08 ; le mandat fixe les distinctions à étudier. | 🟡 |
| Graphe de dépendances AND/OR | S06, S07, S08. Plusieurs accès à une même condition permettent improvisation et robustesse. | 🟡 |
| Topologie spatiale distincte du graphe logique | S08, inféré des croquis et de l'objectif procédural. | 🟡 |
| Gestionnaire de quêtes construit le problème ; agents cherchent la solution | S08. Principe de séparation explicite. | 🟡 |
| Solvabilité initiale et dynamique | S08, S07. La redondance ou alternative des éléments critiques est une règle à valider. | 🟡 |
| Génération procédurale | S06, S08. Finalité, dépendances, placement, informations et validation. | 🟡 |

## B7. Émotions, psychologie et besoins : généalogie retrouvée dans S01

| Élément historique | Contenu retrouvé | Évaluation |
|---|---|---|
| Premières jauges `EmotionState` | stress, confiance envers autrui, confiance en soi, espoir, besoin de sécurité ; plages 0-100 proposées. | 🟠. Une base de réflexion, mais mélange potentiel entre état global, relation et besoin. |
| Modèle Maslow | Physiologie, sécurité, appartenance, estime, sens/accomplissement. | 🟠. Bon outil de questionnement ; ne doit pas imposer une pyramide ou une théorie réaliste de la motivation. |
| Roue de Plutchik | joie, confiance, peur, surprise, tristesse, dégoût, colère, anticipation ; intensités et diades. | 🟠. Référence possible pour vocabulaire, non spécification de simulation. |
| Jauges complémentaires | sécurité/menace, confiance/méfiance, confiance en soi/doute, espoir/désespoir, soutien/isolement, maîtrise/impuissance, clarté/confusion, confort/inconfort, nouveauté/routine, dignité/humiliation, sens/absurdité, moralité/violation, calme/stress. | 🔵. Taxonomie historique à simplifier, tester et séparer par couche. |
| Chaîne événement -> jauges -> émotion -> comportement | Un piège modifie sécurité et stress ; la peur peut conduire à la fuite ou demande d'aide. | 🟡. Très bonne architecture conceptuelle, mais pas de pondération canonique. |
| Pondérations numériques | Matrices et exemples de valeurs `+20`, `0.3`, seuils. | 🟠. Chiffres générés par assistant, sans calibration ni tests ; non utilisables comme valeurs de production. |

---

# C. Doublons, contradictions, ambiguïtés et décisions à prendre

## C1. Synonymes et chevauchements à normaliser

| Terme rencontré | Ambiguïté | Proposition de vocabulaire de travail |
|---|---|---|
| Information / connaissance / mémoire | S01 les place parfois dans l'inventaire et dans une même pile que les besoins. | Information = contenu transmissible ; mémoire = trace vécue/observée ; connaissance = croyance exploitable fondée sur une ou plusieurs sources. |
| but / objectif / but | Le PPT appelle `Find Exit` un but ; le mandat réserve objectif à un résultat souhaité et quête à un graphe. | but = intention ou état désiré de l'agent ; objectif = état opérationnel testable ; quête = structure de problème du niveau. |
| tâche / action | Le whiteboard nomme plusieurs verbes « actions » alors que le mandat demande une tâche planifiable et une action atomique. | tâche = unité planifiée composée ou paramétrée ; action = capacité exécutée sur le monde. |
| Confiance | Est une émotion dans Plutchik, une jauge globale dans le C++ historique et une relation A->B dans les discussions. | confiance relationnel doit être séparé de la disposition générale à faire confiance et de l'émotion d'acceptation. |
| Sécurité / besoin de sécurité / sécurité physique | État du monde, état ressenti et besoin peuvent être confondus. | Sécurité physique = évaluation du danger ; besoin de sécurité = priorité motivationnelle ; état du monde = dangers réellement présents. |
| Psychologie / EmotionState | Dans S01, `EmotionState` contient des variables lentes, sociales et de besoin. | Renommer conceptuellement en couches avant tout code : personality, psychological state, emotion, needs, relationships. |

## C2. Contradictions et tensions majeures

| ID | Constat | Sources | Risque | Décision à valider |
|---|---|---|---|---|
| C-01 | **Inventaire illimité** est explicitement déclaré, tandis que la rareté et les dilemmes supposent une pression de ressource. | S01 | L'inventaire illimité supprime la logistique mais pas la rareté. | 🟡 Garder l'inventaire physique illimité au prototype, mais définir clairement masse, stockage, portabilité ou aucun coût. |
| C-02 | Les NPC ont « les mêmes capacités » que le joueur, mais le joueur peut disposer d'une UI de carte, journal et méta-connaissance. | S05, S06, S07 | Risque d'asymétrie invisible ou de simulation excessive. | Définir les capacités systémiques communes et les privilèges UX du joueur. |
| C-03 | Coopération « jamais obligatoire » contre NPC possesseur de clé/code critique. | S01, S06, S08 | Risque de softlock ou de fausse liberté. | Tout verrou critique doit avoir une alternative, récupération ou équivalent à valider. |
| C-04 | Violences et pièges mortels très larges contre lisibilité et robustesse du puzzle. | S01 | Mort/destruction peut rendre l'état incorrigible ou produire une difficulté arbitraire. | Définir le périmètre de combat, mortalité et persistance avant d'ajouter le catalogue de hazards. |
| C-05 | Les réponses Gemini proposent un modèle émotionnel riche, mais les instructions récentes demandent de ne pas le remplacer ou canoniser. | S01, S08 | Risque de prendre une liste exhaustive pour un modèle implémentable. | Audit séparé par couche puis vertical slice avec peu de variables. |
| C-06 | « Confiance » apparaît à la fois comme émotion, attribut global et relation directionnelle. | S01, S06 | Calculs incohérents et feedback incompréhensible. | Fixer trois noms distincts ou supprimer l'un des concepts. |
| C-07 | Exit/Infernal Escape/Minotaurus apparaissent dans les sources. | S01, S06, S08 | Pollution de noms issue de conversations assistants. | Nom de travail actuel : EXIT. Conserver Infernal Escape comme historique ; exclure Minotaurus sauf décision explicite. |
| C-08 | La V0 suggère l'anti-softlock comme invariant déjà adopté, alors que le mandat demande de l'étudier et valider. | S07, S08 | La Bible risque de figer une règle utile mais coûteuse sans décision. | Classer la règle en WIP, ajouter coûts et exceptions. |

## C3. Questions ouvertes prioritaires

1. **Noyau de prototype :** quels systèmes doivent exister dans le premier vertical slice, et quels systèmes restent simulés ou reportés ?
2. **Mort/incapacité :** quel est le comportement du monde si joueur ou NPC tombe, meurt, fuit, est enfermé ou devient hostile ?
3. **Conflit :** la violence est-elle un dernier recours, un système complet ou une simple menace narrative ?
4. **Connaissances :** quel niveau de mensonge, oubli, contradiction et vérification est nécessaire au plaisir avant de devenir une charge ?
5. **Relations :** quelles dimensions minimales pilotent une coopération convaincante sans tableau de bord illisible ?
6. **Psychologie :** faut-il débuter avec des variables continues, quelques profils lisibles, ou un hybride ?
7. **Temporalité :** temps réel, pause tactique, tour par tour ou système hybride ? Cette décision conditionne exploration, GOAP et pression.
8. **Procéduralité :** générer les graphes, la topologie, le placement, les NPC, ou seulement certaines combinaisons ?
9. **Narration :** le complexe est-il un laboratoire, une base, une prison ou une structure volontairement indéterminée à ce stade ?
10. **Interface :** que sait le joueur explicitement, et que doit-il déduire du monde et du comportement des NPC ?

---

# D. Glossaire conceptuel initial

> Ces définitions sont une proposition de séparation de vocabulaire pour le travail de Phase 2. Elles deviennent CANON uniquement après validation.

| Terme | Définition de travail | Ne pas confondre avec | Statut |
|---|---|---|---|
| État du monde | État réel, autoritatif et simulé du monde : positions, états de portes, contenu des coffres, ressources, dangers et agents. | La connaissance qu'un agent en a. | 🟡 |
| perception | Donnée brute reçue par un agent depuis le monde ou un autre agent. | observation, qui est une inspection interprétée. | 🟡 |
| observation | Résultat d'un acte de regard, recherche, lecture ou inspection. | Vérité garantie : une observation peut être limitée ou ambiguë. | 🟡 |
| mémoire | Enregistrement local d'un événement perçu ou vécu, avec contexte et date. | connaissance, qui peut être consolidé ou déduit. | 🟡 |
| connaissance | Croyance exploitable à propos du monde, avec contenu, provenance, confiance, fraîcheur et vérification possibles. | État du monde ; un besoin ou un objectif. | 🟡 |
| état physique | État corporel d'un agent : santé, blessure, faim et autres variables retenues. | besoin : l'état produit ou module un besoin. | 🟡 |
| personnalité | Dispositions relativement stables qui modulent l'évaluation et la décision. | Émotions rapides et état psychologique évolutif. | 🔵 |
| état psychologique | État interne évoluant lentement : sentiment de sécurité, moral, maîtrise perçue, etc. | personnalité ou une relation à un agent précis. | 🔵 |
| émotion | Réaction contextuelle et relativement brève, exprimable ou non, issue d'une évaluation. | état psychologique durable. | 🔵 |
| besoin | Manque ou condition à satisfaire qui crée de l'urgence ou une pression motivationnelle. | but ou objectif. | 🟡 |
| relation | État directionnel d'un agent envers un autre : historique, proximité, dette, grief et variables retenues. | État psychologique global. | 🟡 |
| confiance | Dimension possible d'une relation : anticipation de fiabilité/intention coopérative de l'autre. | Émotion Plutchik, confiance en soi, confiance générale. | 🟡 |
| but | État désiré ou intention priorisée par un agent. Ex. survivre, s'échapper, protéger un allié. | objectif : résultat opérationnel. | 🟡 |
| objectif | État testable dont l'atteinte contribue à un but ou une quête. Ex. accéder au laboratoire. | tâche, qui décrit une opération. | 🟡 |
| stratégie | Choix d'approche à haut niveau parmi des voies et compromis. Ex. négocier plutôt que forcer. | plan ordonné d'actions. | 🟡 |
| GOAP | Méthode de planification agent-side qui cherche une chaîne d'actions satisfaisant un but à partir de l'état connu. | Gestionnaire de quêtes et graphe auteur du niveau. | 🟡 |
| plan | Séquence ou réseau de Tasks ciblées par un agent, révisable quand ses croyances ou le monde changent. | Stratégie et action exécutée. | 🟡 |
| tâche | Opération planifiable avec acteur, action, cible, préconditions, coût et effets attendus. | action atomique. | 🟡 |
| action | Capacité atomique effectivement tentée dans le monde. Ex. `Take`, `Ask`, `Repair`, `Move`. | objectif ou tâche complète. | 🟡 |
| quête | Graphe d'Objectives et de conditions qui représente un problème du niveau et ses voies de résolution. | Le plan personnel d'un agent. | 🟡 |
| Gestionnaire de quêtes | Système qui compose, instancie, valide et suit les problèmes du niveau ; il connaît la structure globale. | GOAP de l'agent. | 🟡 |
| Softlock | État non terminal où aucune voie vers la progression requise n'est encore disponible. | Échec annoncé ou choix coûteux mais récupérable. | 🟡 |

## Chaîne de référence proposée

```text
WORLD STATE
    -> perception / observation
    -> memory
    -> knowledge et analyse
    -> needs et goals
    -> strategy
    -> GOAP / planning
    -> tasks
    -> actions
    -> WORLD STATE

personnalité, physical state, psychological state, emotions,
relationships et trust modulent cette chaîne sans être fusionnés avec elle.

Gestionnaire de quêtes : compose et valide le problème du monde.
Agent : perçoit une partie du problème et choisit comment le résoudre.
```

---

# E. Table des matières cible de la Bible

La structure privilégie une lecture par système, des liens de dépendance et des fiches évolutives. Chaque chapitre portera un statut, une provenance, une dernière décision et des questions ouvertes.

1. **Registre documentaire** - version, statut, conventions, sources, changelog et décisions
2. **Vision et identité d'EXIT** - pitch, promesse, expérience, non-objectifs
3. **Références et généalogie du projet** - inspirations, sources historiques, éléments écartés
4. **Piliers de design et fun factors**
5. **Core loop et pressure loop**
6. **Monde, contexte et règles de fiction**
7. **Ingrédients systémiques** - taxonomie des objets, machines, ressources, accès et informations
8. **État du monde, temps et règles globales**
9. **joueur capabilities et interactions**
10. **Inventaires, possession et partage**
11. **perception, observation, mémoire et knowledge**
12. **Exploration, carte et découverte**
13. **Puzzles et graphes de dépendances AND/OR**
14. **quête, objectif, tâche et action**
15. **Gestionnaire de quêtes et génération procédurale**
16. **Solvabilité, récupération et anti-softlock**
17. **Survie, dangers et pression**
18. **NPC agents : capacités, boucle cognitive et réévaluation**
19. **GOAP, stratégie et planification agent-side**
20. **personnalité, état psychologique, émotions et besoins**
21. **Relationships, trust, coopération et négociation**
22. **Dialogue et échange d'information**
23. **Choix, dilemmes et conséquences sociales**
24. **Level design, topologie et génération spatiale**
25. **Progression, difficulté et rejouabilité**
26. **Narration et environmental storytelling**
27. **Direction artistique et sonore**
28. **UI, UX et feedback systémique**
29. **Situations exemples et niveau de référence complet**
30. **Architecture conceptuelle et frontières avec Olympe Engine**
31. **Glossaire canonique**
32. **Design backlog**
33. **Registre des décisions et changelog**

## Format standard d'une fiche système

```text
Nom
Statut et provenance
Définition
Intention de design
Expérience joueur
Règles
Entrées et états
Processus
Sorties et effets
Interactions avec les autres systèmes
Exemples
Paramètres à calibrer
Risques et edge cases
Questions ouvertes
Historique et décisions
```

---

# F. Design backlog initial

| ID | Sujet | question | Priorité | Dépendances | Statut |
|---|---|---|---|---|---|
| DB-01 | Noyau de prototype | Quel est le plus petit niveau prouvant puzzle, information, NPC et coopération non forcée ? | Haute | Vision, interactions | 🔴 |
| DB-02 | Taxonomie action | Quelles actions atomiques constituent le noyau jouable et planifiable ? | Haute | Agent, GOAP, objets | 🔴 |
| DB-03 | Connaissances | Quelles propriétés sont indispensables : source, confiance, date, vérification, mensonge, oubli ? | Haute | Dialogue, NPC | 🔴 |
| DB-04 | Mort/incapacité | Quelles transitions empêchent ou autorisent la disparition d'un NPC et d'une ressource critique ? | Haute | Solvabilité, survie | 🔴 |
| DB-05 | Anti-softlock | Quelles garanties, exceptions et outils de détection dynamique sont retenus ? | Haute | Gestionnaire de quêtes | 🔴 |
| DB-06 | confiance | Quels axes relationnels minimaux sont visibles et influencent les décisions ? | Haute | NPC, UI, dialogue | 🔴 |
| DB-07 | Psychologie | Quelles couches initiales sont simulées, avec quelle fréquence et quels effets ? | Haute | NPC, feedback | 🔴 |
| DB-08 | Temps | Quel modèle temporel soutient la pression sans gêner l'observation et le plan ? | Haute | Tout le jeu | 🔴 |
| DB-09 | Procéduralité | Quel contenu est généré et quel contenu est authored ? | Haute | Gestionnaire de quêtes, level design | 🔴 |
| DB-10 | Survie | Faim et santé suffisent-elles au premier slice ; quand introduire soif, fatigue ou stress ? | Moyenne | Ressources, UI | 🔴 |
| DB-11 | Violence | Quel rôle jouable et systémique pour conflit, vol, coercition et armes ? | Moyenne | Relations, sécurité | 🔴 |
| DB-12 | Lore | Quel degré de détermination du complexe est souhaitable avant prototypage ? | Moyenne | Narration, art | 🔴 |

---

# G. Conclusion de Phase 1 et suite recommandée

## Ce qui a été effectivement audité

- Le PDF **Infernal Escape** en entier, 57 pages.
- La présentation historique **EXIT - The Infernal Escape**, 11 diapositives.
- Les deux croquis de dépendances et de gameplay, ainsi que le whiteboard de capacités/boucle cognitive.
- La conversation ChatGPT référencée, dans les limites de son historique accessible.
- La V0 **EXIT_Bible_de_Game_Design.docx** en entier.
- Le mandat de mission actuel.

## Sources manquantes ou inaccessibles

- Le contenu complet de la conversation Gemini publique derrière son lien, hors export PDF.
- Le code complet de l'ancien Quest_Manager et de l'ancien prototype Olympe Engine.
- Une chronologie datée des croquis, itérations et décisions.

## Contradictions majeures détectées

- Mélange historique entre mémoire, connaissance, besoin, objectif et intention.
- Mélange historique entre confiance relationnelle, confiance générale et émotion de confiance.
- Risque de conflit entre NPC autonomes, ressources uniques et coopération non obligatoire.
- Modèle émotionnel historique riche mais non calibré, avec des valeurs numériques non validées et des catégories qui se chevauchent.
- Ambition de hazards/mortalité à arbitrer avec la lisibilité, la justice et l'anti-softlock.

## Décisions qui demandent validation du propriétaire du projet

1. Le noyau du premier vertical slice.
2. Le modèle temporel.
3. Le périmètre de violence et de mortalité.
4. La garantie anti-softlock et son coût de design acceptable.
5. La séparation de vocabulaire proposée, en priorité connaissance/mémoire, but/objectif, tâche/action et les trois sens de confiance.
6. Le niveau de simulation psychologique à viser au prototype.

## Prochaines sections recommandées

1. **Section 01 - Vision, identité et expérience EXIT.** Elle peut être rédigée maintenant à partir du noyau commun, avec chaque affirmation étiquetée WIP ou proposition lorsque nécessaire.
2. **Section 11 - perception, mémoire et knowledge.** C'est la dépendance logique des NPC, de l'échange d'information et du GOAP.
3. **Sections 13 à 16 - Graphes, Gestionnaire de quêtes, solvabilité.** Elles définissent le problème que les agents résolvent.
4. **Sections 18 à 21 - Agents, planification, psychologie et relations.** Elles ne doivent être écrites qu'après la stabilisation du vocabulaire et du premier slice.
