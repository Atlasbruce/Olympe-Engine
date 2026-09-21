# EXIT --- Design Review DR-08

## Vertical Slice de référence --- décisions 01 à 05

**Statut :** 🟢 DR08-01 à DR08-05 validées ; DR08-05 REVIEW COMPLETED\
**Date :** 21 septembre 2026\
**Participants :** Joueur humain, NPC A, NPC B

## 1. État initial et topologie

Les trois participants ignorent topologie, positions et graphe complet.
Les Observers annoncent : « Participants, votre objectif est de trouver
la sortie et de quitter le complexe. » Cette information est commune
mais ne force pas un but. Seule exception : NPC B connaît le code du
générateur. B est légèrement blessé ; cela réduit mobilité/capacités
physiques sans lui imposer une décision.

``` text
                         EXIT
                          │
            GENERATOR ────┼────── SAFE
                │         │
                │      NPC A
                │         │
 COINS ─────────┼──── MEDIKIT
                          │
                          │
                       NPC B
                          │
                     BATTERIES
```

  -----------------------------------------------------------------------
  Élément                             État initial
  ----------------------------------- -----------------------------------
  Joueur                              Salle A, proche du générateur

  NPC A                               Zone/couloir central

  NPC B                               Salle des batteries, légèrement
                                      blessé, code connu

  Ressources                          3 pièces, 2 batteries, 1 medikit à
                                      3 doses, 1 clé cachée dans le safe
  -----------------------------------------------------------------------

## 2. Graphe de quête et branches

``` text
ESCAPE → EXIT → clé → SAFE
SAFE ← énergie + pièce + batterie
énergie ← GENERATOR ← code + pièce
code ← NPC B

Branche sociale optionnelle : MEDIKIT ← énergie + batterie → Heal(NPC B)
```

> Aider NPC B est une décision systémique/sociale, pas une étape
> scriptée de la solution.

## 3. Sémantique des ressources

  -----------------------------------------------------------------------
  Ressource / élément                 Règle CANON
  ----------------------------------- -----------------------------------
  Pièce                               Consommée à l'insertion ; condition
                                      définitivement satisfaite.

  Clé                                 Consommée à l'utilisation sur EXIT.

  Batterie                            Allouée : reste dans le Monde,
                                      récupérable et réallouable ; aucune
                                      décharge dans ce prototype.

  Énergie                             Fournie durablement par le
                                      générateur.

  Code                                Information transmissible, fournie
                                      et non consommée.

  Générateur                          Code + pièce, dans n'importe quel
                                      ordre ; mémorise les deux
                                      conditions ; reste activé et
                                      alimente durablement.

  Safe                                Énergie + pièce + batterie ; reste
                                      ouvert définitivement.

  Medikit                             Énergie + batterie ; trois doses ;
                                      chaque soin consomme une dose ;
                                      batterie récupérable.
  -----------------------------------------------------------------------

## 4. Accès subjectif au Monde --- DR08-01 / 03

**🟢 CANON :** `PERÇU ≠ COMPRIS ≠ INTERACTIF`. Visibilité indicative
15--20 m, observation/lecture ≤2 m, interaction physique ≤0,5 m :
paramètres de tuning, non constantes finales. La grammaire systémique
est commune, mais les exigences spécifiques d'une instance sont
découvertes par observation/lecture.

Au démarrage, chaque participant reconstruit subjectivement le problème
par perception, observation, interaction, expérience et échanges. Un
agent ne planifie jamais à partir d'une information absente de sa
connaissance.

## 5. Exposé, interne et transitions --- DR08-05 D02/D03

`💬` désigne une information ou besoin explicitement exposé ; `💭` un
contenu interne, une connaissance ou un état non directement accessible.
La proximité seule ne convertit jamais 💭 en connaissance d'autrui.

Un contenu physique peut devenir exposé après changement d'état : safe
fermé contenant la clé → safe ouvert, clé objet du Monde visible,
perceptible et collectible. Un participant inconscient conserve
inventaire attaché, découvrable par fouille ; un participant mort expose
son inventaire physique autour du corps. Une connaissance ne devient
jamais un objet : le code non transmis par B peut être définitivement
perdu.

## 6. Concurrence, plans et communication --- DR08-05 D04 à D08

Planifier une ressource ne la réserve pas. Plusieurs participants
peuvent planifier `Take(Key)` ; la première action appliquée au Monde
change l'état, les autres plans incompatibles se réévaluent.
`A la ressource ≠ connaît son usage ou la localisation de sa cible`.

  ---------------------------------------------------------------------------------
  Principe                            Règle CANON
  ----------------------------------- ---------------------------------------------
  Communication spatiale              Direct ≤2 m ; parole ≈5 m ; cri ≈20--30 m,
                                      paramètres 🟡 tuning. Une conversation n'est
                                      pas privée par défaut.

  Réception                           `ENTENDRE ≠ MÉMORISER ≠ CROIRE ≠ RÉPONDRE`.

  Déclaration                         Dire posséder une information ne révèle pas
                                      son contenu.

  Affirmation                         Une affirmation crée une information
                                      attribuée à une source, pas une vérité du
                                      Monde.

  Échange                             Objet⇄objet, objet⇄information,
                                      information⇄information/action,
                                      action⇄objet/information.

  Guidage                             Guider est distinct de révéler une
                                      localisation.

  Accord social                       Proposition, acceptation et attente sociale
                                      forment une expérience mémorisable ; pas une
                                      condition de quête ni un contrat du Monde.
  ---------------------------------------------------------------------------------

### 6.1. Grammaire conceptuelle de communication --- D06

La communication distingue l'existence d'une information de la
divulgation de son contenu. Un participant peut donc exploiter
socialement une connaissance sans nécessairement la révéler.

``` text
ASK
« Sais-tu où se trouve la sortie ? »
« Que demande cet objet ? »

DECLARE
« Je sais où se trouve la sortie. »
« Je connais le code du générateur. »

REVEAL
« La sortie se trouve dans la Salle D. »
« Le code du générateur est 7314. »

REQUEST / PROPOSE
« Dis-moi où se trouve la sortie. »
« Donne-moi la clé et je te conduis à la sortie. »
```

`DECLARE` ne transmet pas le contenu de la connaissance déclarée. Par
exemple, « Je sais où se trouve la sortie » peut produire chez le
destinataire la connaissance `Player claims to know ExitLocation`, mais
ne lui donne pas `ExitLocation`.

Le mode de transmission est orthogonal au contenu de l'acte :

``` text
SPEAK
SHOUT
```

Les noms exacts, l'API et la représentation technique de
`ASK / DECLARE / REVEAL / REQUEST / PROPOSE` restent 🟡 WIP. La
distinction conceptuelle entre demander, déclarer sans divulguer,
révéler et proposer/négocier est 🟢 CANON.

``` text
VÉRITÉ DU MONDE ≠ MA CONNAISSANCE ≠ CE QUE JE DÉCLARE
≠ CE QUE TU ENTENDS ≠ CE QUE TU CROIS ≠ CE QUE TU FAIS
```

Les Observers peuvent annoncer des faits ou diffuser une information
vraie, partielle, ambiguë ou fausse : une annonce est sourcée et
n'altère jamais directement la vérité du Monde.

## 7. Restart, Reload et cohérence

Restart réinitialise Monde et runtime de quête, mais conserve
mémoire/connaissance, empreintes émotionnelles, relations, confiance et
histoire sociale des participants persistants. Reload recrée la
simulation et les instances NPC initiales. Une négociation ou trahison
d'une tentative peut donc influencer une suivante après Restart. Les
frontières DR01→DR07 restent inchangées : aucun lien magique
`Quest Graph → connaissance/plan NPC` n'est introduit.

## 8. Décisions DR08 enregistrées

  -----------------------------------------------------------------------
  ID                      Décision                Statut
  ----------------------- ----------------------- -----------------------
  DR08-01                 Accès subjectif : perçu 🟢 CANON
                          ≠ compris ≠ interactif. 

  DR08-02                 Topologie, ressources   🟢 CANON
                          et sémantique du slice. 

  DR08-03                 État initial et         🟢 CANON
                          reconstruction          
                          subjective du graphe.   

  DR08-04                 Initial State           🟢 CANON
                          consolidé.              

  DR08-05 D01             Accès subjectif à la    🟢 CANON
                          réalité.                

  D02                     Exposé 💬 / interne 💭. 🟢 CANON

  D03                     Transitions             🟢 CANON
                          d'exposition et         
                          inventaire physique.    

  D04                     Plans concurrents,      🟢 CANON
                          aucune réservation,     
                          réévaluation.           

  D05                     Communication spatiale  🟢 CANON
                          multi-participant.      

  D06                     Déclaration sans        🟢 CANON
                          divulgation.            

  D07                     Affirmation ≠ vérité du 🟢 CANON
                          Monde.                  

  D08                     Accords sociaux comme   🟢 CANON
                          expériences             
                          mémorisables.           
  -----------------------------------------------------------------------

## 9. WIP explicitement préservés

Portées finales, actes/API de communication, moteur général d'inférence,
représentation technique des propositions/accords/engagements,
algorithmes d'évolution de confiance/relation/empreinte émotionnelle.
