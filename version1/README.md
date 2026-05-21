# Version 1 — Tubes nommés (FIFO)

## Principe

Un seul client communique avec le serveur via deux FIFOs créés par le serveur dans le répertoire courant : `client2server` (requêtes) et `server2client` (réponses).

## Architecture

```
Client ──[client2server]──▶ Serveur
Client ◀─[server2client]─── Serveur
```

## Protocole en deux phases

Pour chaque requête, le client et le serveur s'échangent d'abord le type de requête (ACK), puis les paramètres :

```
Client          Serveur
  │──── "1" ────▶│   type de requête (1=consultation, 2=réservation)
  │◀─── "1" ─────│   ACK : serveur prêt à recevoir les paramètres
  │──── idx ─────▶│   paramètres de la requête
  │◀─── rép. ────│   résultat
```

## Lancer

Les FIFOs étant créés dans le répertoire courant, serveur et client doivent être lancés depuis le même répertoire :

```bash
cd /tmp/montest
/chemin/vers/build/version1/server &
/chemin/vers/build/version1/client
```

Le serveur supprime les FIFOs à la réception de SIGINT (`Ctrl+C`).

## Limitations

- Un seul client à la fois : le serveur est bloquant entre chaque requête.
- Extension à plusieurs clients non triviale sans fork ou threads.
- Chaque client supplémentaire nécessiterait sa propre paire de FIFOs dédiés : le nombre de canaux croît linéairement avec le nombre de clients, rendant l'architecture non-scalable.
