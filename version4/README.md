# Version 4 — File de messages + 1 thread par requête

## Principe

Trois threads de service écoutent chacun leur `mtype` dans la MSQ. À chaque requête reçue, un **thread worker** est créé dynamiquement pour la traiter, puis libéré (`pthread_detach`). Le thread de service reste immédiatement disponible pour la requête suivante.

## Architecture

```
[MSQ key=17]
  INIT  ──▶ Thread service Init  ──▶ Worker détaché
  CHECK ──▶ Thread service Check ──▶ Worker détaché
  BOOK  ──▶ Thread service Book  ──▶ Worker détaché
```

## Différence avec la version 3

| Aspect | Version 3 | Version 4 |
|--------|-----------|-----------|
| Traitement | Thread de service bloquant | Worker détaché par requête |
| Parallélisme | 1 requête/type à la fois | N requêtes/type en parallèle |

Les workers étant détachés (`pthread_detach`), ils libèrent leurs ressources eux-mêmes à la fin sans nécessiter de `pthread_join`.

## Ressources IPC

Identiques à la version 3 (MSQ key=17, sémaphores keys 18 et 19).

## Lancer

```bash
build/version4/server &
build/version4/client
```
