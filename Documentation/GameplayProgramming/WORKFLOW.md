# Gameplay Programming Knowledge Workflow

## Source of truth by purpose

| Need | Source |
|---|---|
| Capture / classify / rank an idea | Google Sheet |
| Decide / prototype / implement | Jira |
| Explain a validated technique | GitHub Markdown |
| Prove behaviour | Olympe Engine code/tests |

## Lifecycle

| Maturity | Meaning | Jira handling |
|---|---|---|
| Idea | Captured inspiration | Usually no issue |
| Qualified | Relevant, non-duplicate, understandable | Usually no issue |
| Spike | Technical uncertainty to resolve | Story/Task + `gp-spike` |
| Candidate | Worth considering for product architecture | Story + `gp-candidate` |
| Planned | Scope and DoD agreed | Story/Task + `gp-planned` |
| Implemented | Working code/prototype | En cours / Revue + `gp-implemented` |
| Documented | Stable reusable reference | Terminé + `gp-documented` |
| Cancelled | Explicit No-Go | Annulé + reason retained |

## Weekly curation rule

Do **not** create Jira work for every weekly tip. A ticket is created only when one of these is true:
- a prototype or measurement is necessary;
- the pattern solves an identified Olympe/EXIT need;
- an implementation/refactor is actionable;
- a design decision needs traceability.

Weekly tips that remain inspirational stay in the Sheet.
