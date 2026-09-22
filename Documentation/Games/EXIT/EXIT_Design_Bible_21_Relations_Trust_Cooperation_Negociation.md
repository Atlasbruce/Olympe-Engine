# EXIT Design Bible
## 21 - Relations, trust, coopération et négociation

**Version :** 0.1  
**Statut :** 🟡 WIP - coopération 🟢 CANON en DR-01 ; modèle relationnel non validé  
**Dépendances :** 11 connaissance, 18 NPC Agents, 19 GOAP, 20 Personnalité Psychologie Émotions Besoins  
**Dernière mise à jour :** 17 septembre 2026

---

## 1. Intention

Dans EXIT, les autres agents détiennent parfois une information, une ressource, une compétence, un accès ou une présence utile. La coopération est donc une stratégie qui peut réduire un risque ou ouvrir une voie, mais elle ne doit être ni automatique ni transformée en obligation opaque.

Le système relationnel existe pour que les actes aient une mémoire sociale : tenir une promesse, donner une ressource rare, mentir, abandonner un allié ou partager un indice peut modifier les décisions futures. L'objectif est de créer une négociation située et lisible, pas de calculer une sympathie abstraite.

## 2. Principes

1. **Les relations sont directionnelles.** `Relation(Alice -> Bob)` peut différer de `Relation(Bob -> Alice)`.
2. **Un événement s'évalue dans son contexte.** Donner une ration a un sens différent si elle est abondante ou si c'est la dernière ration du donneur.
3. **confiance n'est pas l'unique dimension sociale.** Il ne doit pas absorber gratitude, dette, peur, affinité et sécurité.
4. **La coopération est située.** Elle peut être facultative, avantageuse ou nécessaire si cette nécessité découle de contraintes intelligibles du problème, jamais d'un verrou social arbitraire.
5. **Les raisons doivent être visibles.** Le joueur comprend un refus, une hésitation ou un changement de comportement par les besoins, événements et signaux observables.
6. **La relation modifie la stratégie, pas la vérité du monde.** Une bonne relation peut ouvrir un échange ; elle ne déverrouille pas une porte sans l'action requise.

## 3. Relation directionnelle

```text
Mara -> Joueur
    confiance : sait-il tenir parole ?
    sécurité : est-il dangereux pour moi ?
    dette / gratitude : que lui dois-je ou que m'a-t-il apporté ?
    alignement : ses objectifs paraissent-ils compatibles avec les miens ?

Joueur -> Mara
    peut être différent : le joueur peut la croire compétente
    mais la juger peu fiable ou dangereuse.
```

Une relation concerne une paire d'agents précise. Elle ne remplace pas la disposition générale d'un NPC à faire confiance, ni son état psychologique global.

## 4. Dimensions candidates

| Dimension | question qu'elle représente | Effet possible | Statut |
|---|---|---|---|
| confiance ou fiabilité | Cette personne tient-elle parole et transmet-elle une information fiable ? | Accepter une promesse, croire un code, prêter une ressource. | 🟡 |
| Sécurité relationnelle | Cette personne représente-t-elle une menace ou une protection ? | Accepter proximité, suivre, fuir, demander secours. | 🔵 |
| Gratitude / dette | Un acte significatif crée-t-il une obligation ou reconnaissance ? | Aider en retour, proposer une information, honorer un échange. | 🔵 |
| Affinité | Ai-je une préférence sociale pour cette personne ? | Initier dialogue, partager spontanément, rester proche. | 🔵 |
| Ressentiment | Ai-je subi une blessure, trahison ou humiliation attribuée à cette personne ? | Refuser, se méfier, confronter, éviter. | 🔵 |
| Alignement perçu | Nos objectifs paraissent-ils compatibles ? | Coopérer sur une quête, garder une information, rivaliser. | 🔵 |

Le premier vertical slice ne doit pas implémenter toutes les dimensions. Il doit prouver qu'une relation directionnelle, une trace d'événement et un besoin contextuel modifient une décision sociale.

## 5. confiance : définition de travail

Dans cette Bible, **trust** désigne l'anticipation qu'un agent a de la fiabilité et des intentions coopératives d'un autre agent. Il ne signifie ni l'émotion « confiance » de Plutchik, ni la confiance en soi, ni la sécurité physique du monde.

| confiance élevé | confiance bas |
|---|---|
| L'agent accepte plus facilement une information, une promesse ou un prêt. | L'agent exige une preuve, une contrepartie, refuse ou cherche une alternative. |
| Il peut partager une ressource ou son plan. | Il retient une information ou protège son inventaire. |
| Il réduit le coût perçu de coordination. | Il augmente le risque perçu de coopération. |

confiance ne doit jamais garantir mécaniquement une acceptation. Un NPC affamé peut refuser de donner sa dernière ration à un allié fiable. Inversement, il peut aider un inconnu si le coût est faible ou si l'urgence est extrême.

## 6. Évaluation contextuelle d'un événement

```text
ÉVÉNEMENT
    Bob donne une ration à Alice
        |
        v
ÉVALUATION PAR ALICE
    - mon besoin de nourriture était-il critique ?
    - la ration était-elle rare ?
    - Bob a-t-il fait un sacrifice visible ?
    - quelle est notre relation antérieure ?
    - est-ce compatible avec mes valeurs et mes buts ?
        |
        v
MISE À JOUR RELATIONNELLE ET PSYCHOLOGIQUE
    gratitude / sécurité / trust / soutien ressenti
        |
        v
CONSÉQUENCE FUTURE POSSIBLE
    partager un code, accepter une escorte, réduire le prix d'un échange
```

Cette évaluation explique pourquoi une même action produit des conséquences différentes selon les agents et les circonstances. Elle doit rester modeste au prototype : identifier les facteurs pertinents, pas fabriquer une formule universelle de relations humaines.

## 7. Événements sociaux candidats

| Événement | Conditions contextuelles à évaluer | Effets potentiels |
|---|---|---|
| Don d'une ressource | Rareté, besoin du receveur, sacrifice du donneur. | Gratitude, trust, dette, sécurité. |
| Promesse tenue | Valeur de la promesse, retard, risque supporté. | Fiabilité et alignement perçus augmentent. |
| Promesse rompue | Cause connue, intention, impact sur l'autre. | confiance baisse, ressentiment ou peur augmente. |
| Information partagée | Fiabilité, coût, valeur, exclusivité. | confiance, coopération, knowledge commun. |
| Mensonge découvert | Intention, dommages, contexte de survie. | confiance baisse, ressentiment, prudence. |
| Soin ou sauvetage | Danger, gravité, coût du geste. | Gratitude, sécurité relationnelle, dette. |
| Refus d'aide | Capacité réelle, besoin propre, contrepartie demandée. | Déception, compréhension, ressentiment ou neutralité. |
| Vol ou agression | Gravité, témoin, réparation possible. | Conflit, peur, rupture de coopération. |

Un refus n'est pas une trahison par défaut. Le système doit distinguer « je ne peux pas », « je ne veux pas », « je demande autre chose », « je ne te fais pas confiance » et « je te manipule » lorsque le joueur peut le percevoir.

## 8. Coopération

### Formes de coopération à étudier

| Forme | Description | Exemple | Statut |
|---|---|---|---|
| Information | Transmettre une connaissance ou un indice. | Dire où se trouve un badge. | 🟡 |
| Ressource | Donner, prêter ou échanger un objet. | Ration contre code. | 🟡 |
| action coordonnée | Plusieurs agents réalisent des Actions complémentaires. | L'un tient une porte, l'autre traverse. | 🔵 |
| Escorte / protection | Réduire un danger ou permettre un déplacement. | Accompagner Mara dans une zone risquée. | 🔵 |
| plan commun | Objectif temporairement partagé. | Réparer le générateur pour atteindre l'infirmerie. | 🔵 |
| Relais | Agent A réalise une partie d'un plan, B poursuit. | Ivo apporte le fusible, le joueur répare. | 🔵 |

### Règle de coopération non obligatoire

Une voie sociale peut être la plus économique, la plus sûre ou la plus riche narrativement. Elle ne peut pas être le seul chemin global sans continuité. Le jeu doit préserver l'agence du joueur : négocier, contourner, vérifier, attendre, chercher une autre ressource ou accepter un coût différent.

## 9. Négociation et échanges

### Contrat de proposition

| Élément | Description | Exemple |
|---|---|---|
| Offre | Ce que l'initiateur donne ou promet. | Une batterie, une information, une escorte. |
| Demande | Ce qu'il souhaite recevoir. | Le code du coffre, un badge, une aide. |
| Contexte | Urgence, besoin, danger, relation et alternatives. | Mara est blessée et craint la zone. |
| Garantie | Preuve, dépôt, promesse ou condition de réussite. | Donner le bandage avant de demander le code. |
| Résultat | Acceptation, refus, contre-offre, report ou rupture. | « Je t'indique le code si tu me soignes. » |

### Évaluation de l'acceptation

Un NPC peut accepter une proposition si sa valeur perçue dépasse son coût et son risque. La décision peut considérer :

- besoin satisfait ou but soutenu ;
- valeur et rareté de l'offre ;
- coût de la perte ou du déplacement ;
- trust, sécurité et promesses antérieures ;
- personnalité, état psychologique et tolérance au risque ;
- alternatives connues ;
- urgence et danger immédiat.

Le calcul exact est hors périmètre. L'exigence de design est que le résultat soit justifiable par une ou deux raisons perceptibles, et non par quinze coefficients opaques.

## 10. Promesses et dettes

Une promesse est une information sociale sur une action future : elle doit être formulée, mémorisée, réalisable et vérifiable. Une dette est une conséquence relationnelle possible d'un acte coûteux reçu ; elle n'est pas automatiquement une obligation mécanique.

| Élément | Règle WIP |
|---|---|
| Promesse | L'agent connaît son contenu, son bénéficiaire et sa condition de réalisation. |
| Vérification | L'autre agent peut constater si la promesse a été tenue, rompue ou empêchée. |
| Rupture légitime | Une promesse impossible à tenir à cause d'un danger connu peut réduire trust moins fortement qu'un mensonge. |
| Dette | Sert à modifier une future décision ou contre-offre, sans supprimer les besoins propres de l'agent. |
| Expiration | Une promesse peut devenir obsolète si le contexte change ; cet état doit être signalé. |

## 11. Dialogue et feedback

Le dialogue n'est pas seulement du texte : il expose une intention, une demande, une connaissance ou une condition sociale. Il doit être relié à des Actions et à des états du monde.

| Situation interne | Formulation ou signal possible | retour d'information système |
|---|---|---|
| besoin critique | « J'ai besoin d'un soin avant de bouger. » | Le joueur comprend une condition de coopération. |
| confiance insuffisant | « Je ne te connais pas assez pour te donner le code. » | La cause sociale est explicite. |
| Offre insuffisante | « Cette batterie vaut plus que ça pour moi. » | Le coût de l'échange est lisible. |
| Promesse rappelée | « Tu as dit que tu m'aiderais à traverser. » | La mémoire sociale devient visible. |
| Information incertaine | « Je crois avoir vu un passage, mais je n'en suis pas sûr. » | Source et certitude sont signalées. |
| Danger | Refus, posture de recul, appel à l'aide. | La sécurité modifie le comportement sans formule affichée. |

## 12. Interactions avec les autres systèmes

| Système | Contribution relationnelle | Effet retourné |
|---|---|---|
| connaissance | Source, promesse, mensonge et connaissance partagée sont mémorisés. | La relation module la crédibilité et le partage. |
| Needs | Détermine la valeur d'une aide ou d'un échange. | Une relation peut rendre l'aide plus probable. |
| GOAP | Actions sociales deviennent des Tasks candidates avec coût et préconditions. | Relation et trust modifient le coût et l'acceptation. |
| Psychologie | Sécurité, peur et motivation colorent l'évaluation sociale. | Les actes sociaux peuvent modifier les états durables. |
| Gestionnaire de quêtes | Place les voies sociales, mais ne force pas une issue. | Garantit une continuité si la relation se dégrade. |
| UI/UX | Montre actes, motifs et conséquences observables. | Réduit l'opacité des décisions NPC. |

## 13. Critères du premier vertical slice

| Élément à prouver | Critère observable |
|---|---|
| Directionnalité | Un NPC peut faire davantage confiance au joueur que l'inverse, ou inversement. |
| Acte contextualisé | Donner un soin à un NPC blessé modifie une décision future plus qu'un don inutile. |
| Demande et contrepartie | Un NPC formule une condition de coopération claire. |
| Refus lisible | Le joueur connaît le motif principal du refus. |
| Échange réel | Une information ou ressource change effectivement de propriétaire ou de portée. |
| Alternative | Refuser la coopération n'empêche pas toute progression. |
| Mémoire sociale | Une promesse tenue ou non modifie une seconde interaction. |

## 14. Risques et garde-fous

| Risque | Symptôme | Garde-fou |
|---|---|---|
| Jauge universelle | Tous les actes deviennent un simple + ou - trust. | Séparer dimensions, contexte et raisons d'événement. |
| Social opaque | Le NPC refuse sans possibilité de compréhension ou d'action. | Expliciter motif, besoin ou contre-offre. |
| Coopération obligatoire | Une relation devient la seule clé d'un verrou critique. | Branches OR et continuité de Section 16. |
| Exploit de cadeau | Le joueur répète une action peu coûteuse pour obtenir un effet infini. | Mémoire, saturation, coût contextuel et besoins réels. |
| Personnalité caricaturale | Un trait produit toujours le même dialogue. | Évaluation multi-facteurs et état dynamique. |
| Diplomatie sans enjeu | Les échanges n'affectent ni plans ni ressources. | Lier toute interaction sociale importante à une option réelle. |

## 15. Paramètres à calibrer

- dimensions relationnelles du prototype ;
- événements mémorisés et durée de leur effet ;
- valeur des ressources selon le besoin ;
- conditions d'acceptation, refus et contre-offre ;
- forme des promesses et des vérifications ;
- coût de partage d'information ;
- visibilité des relations dans l'UI ;
- règles de mensonge, de vol et de réparation ;
- saturation des effets d'aide répétée ;
- persistance des relations entre niveaux.

## 16. Décisions de validation demandées

1. Valider le caractère directionnel des relations.
2. Sélectionner les dimensions du premier slice : proposition minimale `trust`, `gratitude ou dette`, et `sécurité relationnelle`.
3. Confirmer que le soin d'un NPC blessé constitue le premier cas de coopération contextualisée.
4. Décider si les contre-offres et promesses sont incluses dans le premier prototype ou documentées pour l'étape suivante.
5. Décider à quel stade mensonge, vol, coercition et violence deviennent jouables.

## 17. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Aide aux besoins, échange d'objets et d'informations, trahison et coopération. | S01 | 🟠 |
| Historique | Confiance et profils de NPC influencent l'aide, la peur et la prise de risque. | S01, S06 | 🟠 |
| Actuel | Relations asymétriques et évaluation contextuelle d'un acte demandées explicitement. | S08 | 🟡 |
| Actuel | Coopération stratégique, non obligatoire ; information comme ressource. | S06, S08 | 🟡 |
| Futur | Formules, poids, UI détaillée et catalogue de dialogue doivent être validés par prototypage. | Audit Phase 1 | 🔵 |

## Propagation DR-02

**🟢 CANON lexical :** `Relation(A→B)` décrit le lien directionnel de A envers B ; `Confiance(A→B)` est l'estimation contextuelle de la fiabilité de B comme source, partenaire, acteur ou engagement. La confiance n'est ni affection, coopération, sécurité, acceptation, permission, ni seuil de comportement. Le modèle social détaillé reste WIP.

## Propagation DR-05

**🟢 CANON :** les événements sociaux sont évalués par les systèmes IA avant toute évolution relationnelle. Leur portée psycho-émotionnelle peut contribuer à cette évaluation : une aide reçue dans une situation critique peut être plus significative que le même acte en sécurité. L'information et la mémoire ne modifient jamais directement la confiance ; erreur, obsolescence et changement du Monde restent des explications possibles.

## Propagation DR-06

**🟢 CANON :** relation, confiance, rappel d'expérience et valeurs morales peuvent converger ou entrer en conflit dans l'appréciation d'une option. Aucun ne prescrit seul entraide, vengeance, acceptation ou refus.

## Propagation DR-08

**🟢 CANON :** une proposition acceptée crée une attente sociale et une expérience mémorisable, non une règle du Monde ni une condition de quête. Une promesse non tenue ne prouve pas automatiquement une intention de trahir ; l'interprétation reste subjective et fondée sur les informations accessibles.

## Propagation DR08-06

**🟢 CANON :** proposition → acceptation → accord → engagement(s), tout en restant distinct de but, plan et action. Une proposition sans réponse est non bloquante. Une menace est une communication conditionnelle : elle ne prouve pas la capacité, ne garantit pas l'action future et ne force jamais la conformité.
