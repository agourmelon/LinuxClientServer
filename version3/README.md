# Version 3 — File de messages + 3 threads permanents

## Principe

Le serveur crée trois threads permanents, chacun dédié à un type de requête. Chaque thread consomme uniquement les messages de son `mtype` dans la MSQ, permettant un traitement parallèle des consultations et des réservations.

## Architecture

```
[MSQ key=17]
  mtype=1 (INIT)  ──▶ Thread Init
  mtype=2 (CHECK) ──▶ Thread Consultation  ┐ accès concurrent à la table
  mtype=3 (BOOK)  ──▶ Thread Réservation   ┘ → synchronisation requise
```

## Synchronisation : lecteurs-écrivains

La table des spectacles est partagée en mémoire entre les threads. Un schéma **lecteurs-écrivains** par sémaphores System V garantit la cohérence :

- Plusieurs consultations simultanées sont autorisées.
- Une réservation exclut toute autre opération concurrente (lecture ou écriture).

Invariant clé dans `checkShowSync` :
- premier lecteur (nbCheckers passe de 0 à 1) → acquiert le verrou d'écriture
- dernier lecteur (nbCheckers repasse à 0) → libère le verrou d'écriture

## Ressources IPC

| Ressource | Clé |
|-----------|-----|
| File de messages | 17 |
| Sémaphores (compteur lecteurs) | 18 |
| Sémaphores (verrou réservation) | 19 |

Le serveur détruit toutes les ressources à la réception de SIGINT.

## Lancer

```bash
build/version3/server &
build/version3/client
```

## Limitation

Chaque thread de service traite une requête à la fois : si un thread de consultation est occupé, les autres requêtes du même type attendent, même si des ressources CPU sont disponibles.
