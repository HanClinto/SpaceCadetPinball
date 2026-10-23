# A shared high score table for the web version

Everyone who plays on the same page shares one high score table: after each game a dialog says what happened ("New
personal best! You passed sam! #2 all-time.") and cycles through **Personal bests** (everyone once), **All-time**
(each player's top 5 games, so one player can't fill it) and **Your games**. The game's own "High Score 1-5" messages
show the shared top 5. XP and Full Tilt Space Cadet each have their own table. Only one-player games without cheats count.

- `scoreboard.js` is the page's side: it is the game's `Module.scoreboard` (see the main README), and talks to a scores
  service over a small JSON API.
- `server.py` is a sample service: Python 3.8+ only, scores in SQLite. It also serves the built game, with
  `scoreboard.js` added to its page.

## Try it

```sh
python3 Platform/Web/scoreboard/server.py bin                   # after building; then open http://localhost:8000/
python3 Platform/Web/scoreboard/server.py bin --host 0.0.0.0    # to play from phones on the same network too
```

Players type a name the first time they finish a game (the page remembers it; "change" in the dialog). That's the
honor system: anyone could post any score under any name, which is fine among friends but not for the open
internet. Behind a reverse proxy that signs people in, pass the header it sets for who's signed in, and names come
from there instead: `--user-header Remote-User`.

## On your own site

Load `scoreboard.js` before the game's script, and point it at your service:

```html
<script src="scoreboard.js" data-api="/scores/api" data-scores-page="/scores/"></script>
<script async src="SpaceCadetPinball.js"></script>
```

`data-api` is where the API is (default `scores/api`, relative to the page), `data-boards` names the XP and Full Tilt
boards (default `spacecadet,spacecadet-ft`), and `data-scores-page` adds a link to a page listing every score. Any
service with the same API works; `server.py`'s docstring describes it. This one runs a family game site, behind its
sign-in, with a Flask version of the same service.
