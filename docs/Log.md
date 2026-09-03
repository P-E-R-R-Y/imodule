# imodule — journal

Marqueurs : 🟢 ajout · 🔴 rupture · 🔵 correctif · ⚪ interne ou doc · 🟡 propose
dans le plan, code non ecrit.

## v0.1.0

- 🟢 `IModule` : `type()`, `name()`, `bind()`, `registry()`, `acquire()`,
  `release()`, `uses()`, `mustClose()`, `isClosed()`, `condemn()`, `reset()`
- 🟢 `IModuleRegistry` : `Get`, `GetAllByType`, `GetAllByKey`, `GetAll`,
  `Current`, `Select`
- 🟢 `accepts[]` : la convention, declaree par chaque interface qui en a
  besoin (`igraphic`)
- 🟢 13 tests sur fausses abstractions (`DummyModule`, `DummyRegistry`)

## v0.2.0 — propose, rien de tout ceci n'est ecrit

- 🟡 `claims()` sur `IModule` — les ressources qu'un module confisque au
  processus, en chaines opaques comparees par egalite
- 🟡 `acquire()`/`release()`/`uses()`/`condemned()` deplaces sur
  `IModuleRegistry` — la table cesserait de croire le module sur parole
- 🟡 `Select()` retire de la vue invite — un invite ne pourrait plus
  re-pointer le contrat de tout le monde
- 🟡 `reset()` supprime — il n'existait que parce que l'etat vivait dans
  une dll que macOS ne demappe pas
