# OLYMPE EDITOR UX GUIDELINES
**Version 2.0 — 2026-03-11**

---

## Table des Matières

1. [Introduction & Philosophie de Design](#1-introduction--philosophie-de-design)
2. [Architecture Multi-Documents (Tab System)](#2-architecture-multi-documents-tab-system)
3. [CRUD Operations & Menus Contextuels](#3-crud-operations--menus-contextuels)
4. [Undo/Redo System](#4-undoredo-system)
5. [Save & Persistence Management](#5-save--persistence-management)
6. [Canvas & Node Positioning](#6-canvas--node-positioning)
7. [Asset Browser Integration](#7-asset-browser-integration)
8. [Inspector & Properties Panel](#8-inspector--properties-panel)
9. [Raccourcis Clavier Complets](#9-raccourcis-clavier-complets)
10. [Diagrammes & Workflows Critiques](#10-diagrammes--workflows-critiques)
11. [Migration & Rétrocompatibilité](#11-migration--rétrocompatibilité)
12. [Bugs Critiques Résolus](#12-bugs-critiques-résolus-lessons-learned)
13. [Thèmes & Accessibilité](#13-thèmes--accessibilité)
14. [Anti-Patterns & Best Practices](#14-anti-patterns--best-practices)
15. [Debugging & Troubleshooting](#15-debugging--troubleshooting)

---

## 1. Introduction & Philosophie de Design

### 1.1 Vision

L'éditeur Olympe vise à offrir une expérience professionnelle comparable aux outils AAA (Unreal Engine, Unity) tout en restant accessible et performant.

**Principes fondamentaux** :
- **Zéro perte de données** : Confirmations avant actions destructives
- **Feedbacks immédiats** : Dirty flags, validations visuelles
- **Standards respectés** : Raccourcis Ctrl+S, Ctrl+Z universels
- **Performance** : Context switching <16ms, 60 FPS minimum

### 1.2 Public Cible

- **Game Designers** : Création graphes AI/dialogues sans programmer
- **Tech Artists** : Prototypage rapide systèmes visuels
- **Level Designers** : Scripting événements in-game

### 1.3 Standards de Référence

| Standard | Source | Application Olympe |
|----------|--------|-------------------|
| Multi-document tabs | VS Code, Chrome | TabManager avec dirty flags |
| Node-based editing | Unreal Blueprint | ImNodes canvas avec CRUD |
| Asset management | Unity Project | AssetBrowser avec double-clic |

---

## 2. Architecture Multi-Documents (Tab System)

### 2.1 Vue d'Ensemble

**Problème Initial** :
- ❌ "New Graph" écrasait le graphe en cours → perte de données
- ❌ Impossible d'éditer plusieurs graphes simultanément
- ❌ Aucune notion de "document ouvert"

**Solution Implémentée** :