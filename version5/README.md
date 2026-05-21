# Version 5 — Sockets TCP (bonus)

## Principe

Clients et serveur peuvent s'exécuter sur des machines différentes. Il s'agit d'une architecture de **serveur parallèle** (par opposition au serveur itératif des versions précédentes) : le serveur crée un **processus fils** (`fork`) pour chaque connexion entrante et retourne immédiatement à la boucle `accept`. Plusieurs clients sont ainsi servis simultanément sans se bloquer mutuellement. L'état partagé (table des spectacles) réside en **mémoire partagée** (shmem) accessible par tous les fils.

## Architecture

```
Client A ─┐
Client B ─┼──▶ Serveur (boucle accept)
Client C ─┘         │
               fork() par client
                     └──▶ Processus fils → runService()
                              └── accès à la shm via sémaphores
```

## Différences avec les versions précédentes

| Aspect | v1–v4 | v5 |
|--------|-------|----|
| Transport | IPC local (tubes / MSQ) | TCP (réseau) |
| Concurrence serveur | threads | processus (`fork`) |
| État partagé | mémoire du processus | mémoire partagée (shmem) |
| Multi-machine | non | oui |
| Déconnexion propre | SIGINT | option `3` (EXIT) + SIGINT |

Les processus zombies sont évités via `signal(SIGCHLD, SIG_IGN)` : le noyau recycle automatiquement les fils terminés sans `wait()`.

## Ressources IPC

| Ressource | Clé |
|-----------|-----|
| Mémoire partagée (spectacles) | 256 |
| Mémoire partagée (nb lecteurs) | 257 |
| Sémaphores (compteur lecteurs) | 18 |
| Sémaphores (verrou réservation) | 19 |

Le processus parent détruit toutes les ressources à la réception de SIGINT. Les fils se contentent de se détacher de la mémoire partagée.

## Lancer

```bash
# Serveur (écoute sur le port 2058)
build/version5/server

# Client (se connecte à 127.0.0.1:2058 par défaut)
build/version5/client
```

Pour déployer sur deux machines, modifier la constante `SERVER` dans `client.c` avec l'adresse IP du serveur, puis recompiler.
