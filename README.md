# Réservation de Spectacles — Projet IPC Linux

Application client-serveur de réservation de places de spectacles, déclinée en cinq versions illustrant progressivement les mécanismes de communication inter-processus (IPC) Linux.

## Le projet

Un serveur maintient une table de spectacles (titre, places restantes). Des clients émettent deux types de requêtes :

- **Consultation** : nombre de places disponibles pour un spectacle donné
- **Réservation** : réserver N places pour un spectacle donné

## Structure du projet

```
.
├── Makefile
├── versionX/           # code source + README de chaque version
│   ├── README.md
│   ├── booking.h       # table des spectacles et fonctions métier
│   ├── message.h       # structures de messages IPC (v2–v4)
│   ├── client.c
│   ├── server.c
│   ├── service.h       # logique de traitement des requêtes (v3–v5)
│   ├── sync.h          # synchronisation lecteurs-écrivains (v3–v5)
│   └── worker.h        # fonctions des threads workers (v4)
└── build/versionX/     # binaires compilés (généré)
    ├── server
    └── client
```

## Compilation

```bash
make VERSION=version1          # compile server et client de la version 1
make clean VERSION=version1    # supprime les binaires de la version 1
```

## Versions

| Version | Mécanisme IPC | Clients simultanés | Concurrence côté serveur |
|---------|--------------|-------------------|--------------------------|
| [version1](version1/README.md) | Tubes nommés (FIFO) | 1 | aucune |
| [version2](version2/README.md) | File de messages (MSQ) | N | 1 thread |
| [version3](version3/README.md) | File de messages (MSQ) | N | 3 threads permanents |
| [version4](version4/README.md) | File de messages (MSQ) | N | 1 thread par requête |
| [version5](version5/README.md) | Sockets TCP | N | 1 processus par client |
