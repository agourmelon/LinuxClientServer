# Version 2 — File de messages System V

## Principe

Plusieurs clients s'exécutent en parallèle. Toutes les requêtes transitent par une file de messages (MSQ) unique. Le serveur les traite en FIFO dans un seul thread.

## Architecture

```
Client A ─┐
Client B ─┼──▶ [MSQ key=17] ──▶ Serveur (1 thread)
Client C ─┘
```

## Routage des réponses

La MSQ est partagée par tous : chaque requête porte le PID du client émetteur. Le serveur répond avec `mtype = clientPid`, ce qui permet à chaque client de filtrer ses propres réponses via `msgrcv(..., pid, 0)`. Un seul canal suffit pour N clients.

## Ressources IPC

| Ressource | Clé |
|-----------|-----|
| File de messages | 17 |

Le serveur détruit la file à la réception de SIGINT.

## Lancer

```bash
build/version2/server &
build/version2/client   # répéter dans autant de terminaux que souhaité
```

## Limitation

Le serveur traite les requêtes séquentiellement : une requête longue bloque toutes les autres.
