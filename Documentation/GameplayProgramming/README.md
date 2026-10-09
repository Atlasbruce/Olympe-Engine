# Gameplay Programming Knowledge Base

This folder is the canonical technical knowledge base for reusable gameplay-programming patterns in Olympe Engine.

## Operating model

- **Google Sheet — Gameplay Programming Tips**: catalog, qualification, priority and maturity.
- **Jira KAN**: decision, spikes, planning and implementation work.
- **GitHub Markdown**: stable technical reference after a pattern is validated.
- **Olympe Engine code**: implementation evidence.

Lifecycle:

`Idea → Qualified → Spike → Candidate → Planned → Implemented → Documented`

Jira keeps its existing operational statuses:

`À faire → En cours → Revue en cours → Terminé(e)`

The lifecycle is carried by the stable GP identifier and Jira labels such as `gp-spike`, `gp-candidate`, `gp-planned`, `gp-implemented`, and `gp-documented`.

## Identifier convention

| Prefix | Area |
|---|---|
| GP-ARCH | Architecture |
| GP-AI | AI / NPC behaviour |
| GP-NAV | Navigation |
| GP-ANIM | Animation |
| GP-UI | UI / menus |
| GP-PERF | Performance |
| GP-INT | Interaction |

Example: `GP-AI-007 — Resource Reservation`.

The same ID must appear in the Sheet, Jira issue title and Markdown document.

## Documentation structure

```text
GameplayProgramming/
├── README.md
├── _PATTERN_TEMPLATE.md
├── Architecture/
├── AI/
├── Navigation/
├── Animation/
├── Interaction/
├── UI/
└── Performance/
```

Create a canonical pattern page only when the knowledge is sufficiently stable. Spikes and unresolved ideas stay in the Sheet/Jira until validated.

## Promotion criteria

A pattern can move to **Documented** when:
1. the problem and intended scope are clear;
2. the technique has been validated by prototype, implementation or strong design evidence;
3. limitations and failure modes are recorded;
4. Olympe/EXIT applicability is explicit;
5. the Jira item links to the canonical Markdown page.
