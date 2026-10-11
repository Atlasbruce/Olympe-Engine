# Animation Graph — checklist de convergence Framework

Cette checklist est le contrat de finition de l'Animation Graph. Elle reprend
les comportements validés dans Visual Script et Behavior Tree : le Framework
reste propriétaire du canvas partagé, de l'historique, du clipboard et des
modales ; l'Animation Graph fournit uniquement son modèle et ses opérations
spécifiques.

## 1. Socle Framework

- [x] `CanvasFramework` unique par document ouvert.
- [x] Barre d'outils partagée : sauvegarde, Save As, Browse, Verify, Run,
  Undo/Redo.
- [x] Modales rendues une seule fois par le gestionnaire central de tabs.
- [x] Canvas partagé : pan, zoom, grille, minimap, Reset View et persistance
  de la vue.
- [x] Historique Framework, sans reprise du legacy Undo/Redo.
- [ ] Les commandes de toutes les actions éditoriales passent par cet
  historique ; aucun changement direct pendant un drag ou un popup.

## 2. Sélection et manipulation

- [x] Clic gauche : sélection simple d'un state.
- [x] Ctrl+clic : ajout/retrait à la sélection.
- [x] Rectangle de sélection ; Ctrl conserve la sélection existante.
- [x] Ctrl+A sélectionne tous les states.
- [ ] Déplacer un groupe sélectionné déplace tous ses states dans une seule
  commande Undo/Redo.
- [ ] Delete supprime la sélection, avec toutes les transitions internes ou
  incidentes, dans une seule commande Undo/Redo.
- [ ] La sélection reste cohérente après création, collage, Undo et Redo.

## 3. Clipboard partagé

- [ ] Ctrl+C et menu canvas copient states, propriétés et transitions internes.
- [ ] Ctrl+V et menu canvas collent au curseur, avec nouveaux identifiants et
  sélectionnent le groupe collé.
- [ ] Le clipboard est transportable entre tabs Animation Graph compatibles.
- [ ] Chaque collage est une commande Undo/Redo atomique.

## 4. States, pins et transitions

- [x] Pins In/Out visibles et création d'une transition par glisser-déposer.
- [x] Sélection de transition au clic gauche.
- [x] Clic droit sur lien sans effacer la sélection active ; menu lien.
- [x] Édition auto-validée des propriétés de transition à la perte de focus.
- [x] Undo/Redo et rafraîchissement immédiat de l'inspecteur de transition.
- [ ] Menus contextuels homogènes : canvas, state, transition et listes.
- [ ] Suppression depuis tout menu contextuel, avec fermeture fiable du popup.
- [ ] Snap de liens/pins et géométrie Bézier lisible à toutes distances.

## 5. Persistance et robustesse

- [x] Sérialisation des states, transitions, propriétés et sources.
- [x] Import TSX vers clips d'une Animation Bank, y compris frames animées.
- [ ] Validation explicite des références de clips, transitions, état par défaut
  et données d'animation manquantes.
- [ ] Sauvegarde/chargement ne conserve aucune sélection ou interaction
  transitoire invalide.
- [ ] Save As sans collision d'identifiants ImGui.

## 6. Parcours Animation Pipeline

- [ ] Animation Bank : inspection des clips, directions, loop, FPS et frames.
- [ ] État : choix du clip, paramètres de lecture et aperçu local.
- [ ] Timeline : évènements images/franchissements, pistes et édition.
- [ ] Preview : lecture de l'état et simulation de transitions.
- [ ] Export runtime : artefact compact consommable par ECS/AI/animation.

## Ordre de réalisation

1. Stabiliser suppression multiple et déplacer une sélection.
2. Ajouter le clipboard partagé Animation Graph.
3. Finaliser menus, raccourcis et feedback de sélection.
4. Compléter validation/persistance.
5. Construire Animation Bank, timeline, preview et export runtime.

Un jalon ne passe à l'étape suivante qu'après le test manuel correspondant :
création, modification, Undo, Redo, sauvegarde, rechargement et vérification
du graphe.
